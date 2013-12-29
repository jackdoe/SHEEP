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


class Sheep {
public:
    string root;
    int shards;
    int counter;

    Sheep(char * _root, int _shards) {
        this->root = string(_root);
        this->shards = _shards;
        this->counter = 0;
    }

    ~Sheep() { std::cout << "Destruction is a way of life for me.\n"; }

    void foo(int &i) {
        i++;
    }
    
    const char * fs_shard_path(int i) {
        fs::path dir (this->root);
        fs::path file (boost::lexical_cast<string>(i));
        fs::path full_path = dir / file;
        return full_path.c_str();
    }

    void _search_into(Query *q, IndexSearcher *s, Hits **h) {
        *h = s->search(q);
    }

    SV *search(SV *query, int n) {
        BooleanQuery *q = _CLNEW BooleanQuery();
        add_query(q,query,BooleanClause::MUST);

        boost::thread t[this->shards];
        IndexSearcher *searchers[this->shards];
        Hits *hits[this->shards];
        int shard;
        for (shard = 0; shard < this->shards; shard++) {
            searchers[shard] = _CLNEW IndexSearcher(FSDirectory::getDirectory(this->fs_shard_path(shard)));
            t[shard] = boost::thread(&Sheep::_search_into, this, q,boost::ref(searchers[shard]),&hits[shard]);
        }

        AV *ret = newAV();
        for (shard = 0; shard < this->shards; shard++) {
            t[shard].join();
//            die("AAA: %d",hits[shard]->length());
            for (size_t i = 0; i < std::min((size_t )n,hits[shard]->length()); i++ ){
                Document* doc = &hits[shard]->doc(i);
                DocumentFieldEnumeration *fields = doc->fields();
                HV *item = newHV();
                while (Field *f = fields->nextElement()) {
                    SV *f_val = newSV(0);
                    SV *f_name = newSV(0);
                    WCharToSv(f->name(),f_name);
                    WCharToSv((const TCHAR *)f->stringValue(),f_val);
                    hv_store_ent(item,f_name,f_val,0);
                }
                av_push(ret,newRV((SV *)item));
            }
        }

        for (shard = 0; shard < this->shards; shard++) {
            searchers[shard]->close();
            _CLLDELETE(searchers[shard]);
        }

        _CLLDELETE(q);
        return newRV((SV *)ret);
    }

    int index(AV *docs) {
        int i,inserted = 0;
        try {
            lucene::analysis::WhitespaceAnalyzer an;
            IndexWriter *writers[this->shards];
            for (i = 0; i < this->shards; i++) {
                bool should_create = IndexReader::indexExists(this->fs_shard_path(i)) ? false : true;
                FSDirectory *dir = FSDirectory::getDirectory(this->fs_shard_path(i),should_create,NULL);
                writers[i] = _CLNEW IndexWriter(dir ,&an, should_create);
                writers[i]->setUseCompoundFile(false);
            }

            Document doc;
            HE *he;
            for (i=0; i<=av_len(docs); i++) {
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
                    doc.add( *_CLNEW Field(w_key, w_val, Field::STORE_YES | Field::INDEX_TOKENIZED) );
                    Safefree(w_key);
                    Safefree(w_val);
                }
                writers[this->counter++ % this->shards]->addDocument( &doc );
                inserted++;
            }

            for (i = 0; i < this->shards; i++) {
                writers[i]->setUseCompoundFile(true);
                writers[i]->optimize();
                writers[i]->close();
                _CLLDELETE(writers[i]);
            }
        } catch (CLuceneError& e) {
            die(e.what());
        }
        return inserted;
    }
};