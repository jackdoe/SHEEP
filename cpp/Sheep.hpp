#include <iostream>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include "CLucene.h"
#include "cpp/Util.hpp"
#include "cpp/Query.hpp"
namespace fs = boost::filesystem;

using namespace lucene::index;
using namespace lucene::search;
using namespace lucene::analysis;
using namespace lucene::util;
using namespace lucene::store;
using namespace lucene::document;
using namespace std;
#define MAX_SHARDS 128

class Sheep {
public:
    string root;
    int shards;
    int counter;
    FSDirectory *directories[MAX_SHARDS];
    lucene::analysis::WhitespaceAnalyzer an;

    Sheep(char * _root, int _shards) {
        if (_shards >= MAX_SHARDS)
            die("%d > MAX_SHARDS(%d)",_shards,MAX_SHARDS);

        this->root = string(_root);
        this->shards = _shards;
        this->counter = 0;
        int shard;
        for (shard = 0; shard < this->shards; shard++) {
            fs::path dir(this->root);
            dir /= (boost::lexical_cast<string>(shard));
            directories[shard] = FSDirectory::getDirectory(dir.c_str(), false, NULL);
        }
    }

    ~Sheep() {
        int shard;
        for (shard = 0; shard < this->shards; shard++) {
            if (this->directories[shard]) {
                this->directories[shard]->close();
                _CLLDELETE(this->directories[shard]);
            }
        }
    }

    void _search_into(Query *q, IndexSearcher *s, Hits **h) {
        try {
            *h = s->search(q);
        } catch (CLuceneError& e) {
            std::cout << e.what();
            *h = NULL;
        }
    }

    SV *search(SV *query, int n) {
        BooleanQuery *q = _CLNEW BooleanQuery();
        add_query(q,query,BooleanClause::MUST);
        IndexSearcher *searchers[this->shards];
        boost::thread t[this->shards];
        Hits *hits[this->shards];
        Document::FieldsType::const_iterator fit;
        const Document::FieldsType* fields;
        int shard;

        AV *ret = newAV();
        for (shard = 0; shard < this->shards; shard++) {
            searchers[shard] = _CLNEW IndexSearcher(directories[shard]);
            t[shard] = boost::thread(&Sheep::_search_into, this, q,searchers[shard],&hits[shard]);
        }

        for (shard = 0; shard < this->shards; shard++) {
            t[shard].join();

            if (!hits[shard]) {
                av_undef(ret);
                die("search error: look at %s:%d, there should also be an error in stdout",__FILE__,__LINE__);
            }
            for (size_t i = 0; i < std::min((size_t )n,hits[shard]->length()); i++ ){
                Document* doc = &hits[shard]->doc(i);
                fields = doc->getFields();
                HV *item = newHV();
                fit = fields->begin();
                while (fit != fields->end()) {
                    Field* field = *fit;
                    SV *f_val = newSV(0);
                    SV *f_name = newSV(0);
                    WCharToSv(field->name(),f_name);
                    WCharToSv((const TCHAR *)field->stringValue(),f_val);
                    hv_store_ent(item,f_name,f_val,0);

                    fit++;
                }
                hv_store(item,"_score",6, newSVnv(hits[shard]->score(i)),0);
                av_push(ret,newRV_noinc((SV *)item));
            }
            _CLLDELETE(hits[shard]);
            searchers[shard]->close();
            _CLLDELETE(searchers[shard]);
        }

        _CLLDELETE(q);
        return newRV_noinc((SV *)ret);
    }

    int index(AV *docs) {
        int i,inserted = 0, shard;
        try {
            IndexWriter *writers[this->shards];
            for (shard = 0; shard < this->shards; shard++) {
                bool should_create = IndexReader::indexExists(this->directories[shard]) ? false : true;
                writers[shard] = _CLNEW IndexWriter(this->directories[shard],&an, should_create);
                writers[shard]->setUseCompoundFile(false);
            }

            Document doc;
            HE *he;
            for (i=0; i<= av_len(docs); i++) {
                SV** elem = av_fetch(docs, i, 0);
                if (elem == NULL || *elem == &PL_sv_undef || !SvROK(*elem) || (SvTYPE(SvRV(*elem)) != SVt_PVHV) )
                    continue;

                HV *hv = (HV *)SvRV(*elem);
                doc.clear();
                while (he = hv_iternext(hv)) {
                    I32 len;
                    char *key = hv_iterkey(he,&len);
                    SV *val = hv_iterval(hv,he);
                    wchar_t *w_key = CharToWChar(0,(U8 *) key,len);
                    wchar_t *w_val = SvToWChar(val);
                    doc.add(*_CLNEW Field(w_key, w_val, Field::STORE_YES | Field::INDEX_TOKENIZED) );
                    Safefree(w_key);
                    Safefree(w_val);
                }
                writers[this->counter++ % this->shards]->addDocument( &doc );
                inserted++;
            }
            for (shard = 0; shard < this->shards; shard++) {
                writers[shard]->setUseCompoundFile(true);
                writers[shard]->optimize();
                writers[shard]->close();
                _CLLDELETE(writers[shard]);
            }
        } catch (CLuceneError& e) {
            die(e.what());
        }
        return inserted;
    }
};
