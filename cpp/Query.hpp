#include "CLucene.h"

using namespace lucene::index;
using namespace lucene::search;
using namespace lucene::analysis;
using namespace lucene::util;
using namespace lucene::store;
using namespace lucene::document;

BooleanQuery *add_query(BooleanQuery *parent, SV *query, BooleanClause::Occur occur);
BooleanQuery *add_queries_from_ar(BooleanQuery *parent, SV *_queries, BooleanClause::Occur occur);

BooleanQuery *add_queries_from_ar(BooleanQuery *parent, SV *_queries, BooleanClause::Occur occur) {
    AV *queries = (AV *)SvRV(_queries);
    int i;

    for (i=0; i<=av_len(queries); i++) {
        SV** elem = av_fetch(queries, i, 0);
        if (elem == NULL || *elem == &PL_sv_undef || !SvROK(*elem) || (SvTYPE(SvRV(*elem)) != SVt_PVHV))
            die("bool query expects hashref");

        add_query(parent,*elem,occur);
    }
    return parent;
}

BooleanQuery *add_query(BooleanQuery *parent, SV *query, BooleanClause::Occur occur) {
    HE *he;
    if (query == NULL || !SvROK(query) || (SvTYPE(SvRV(query)) != SVt_PVHV) )
        die("add_query expects hashref");

    HV *h_query = (HV *)SvRV(query);
    while (he = hv_iternext(h_query)) {
        I32 len;
        char *key = hv_iterkey(he,&len);
        SV *val = hv_iterval(h_query,he);
        if (val == &PL_sv_undef || val == NULL)
            die("undef in the query");
        if (!SvROK(val) || (SvTYPE(SvRV(val)) != SVt_PVHV) )
            die("expecting hashref");
        HV *h_val = (HV *)SvRV(val);
        HE *he_val;
        if (strncmp("term",key,len) == 0) {
            while (he_val = hv_iternext(h_val)) {
                I32 len;
                char *t_key = hv_iterkey(he_val,&len);
                SV *t_val = hv_iterval(h_val,he_val);
                wchar_t *w_key = CharToWChar(0,(U8 *) t_key,len);
                wchar_t *w_val = SvToWChar(t_val);
                Term *t = _CLNEW Term(w_key, w_val);
                parent->add(_CLNEW TermQuery(t),true,occur);
                Safefree(w_key);
                Safefree(w_val);
                _CLDECDELETE(t);
            }
        } else if (strncmp("bool",key,len) == 0) {
            SV **pp_should = hv_fetch(h_val,"should",6,0);
            SV **pp_must = hv_fetch(h_val,"must",4,0);
            SV **pp_must_not = hv_fetch(h_val,"must_not",8,0);

            #define PP_IS_NOT_AREF(elem) ((elem) != NULL && (!SvROK(*(elem)) || (SvTYPE(SvRV(*(elem))) != SVt_PVAV)) )
            if (PP_IS_NOT_AREF(pp_should) || PP_IS_NOT_AREF(pp_must) || PP_IS_NOT_AREF(pp_must_not))
                die("bool query expects { must => [], should => [], must_not => [] }");

            BooleanQuery *b = _CLNEW BooleanQuery();
            if (pp_should)
                add_queries_from_ar(b,*pp_should,BooleanClause::SHOULD);
            if (pp_must)
                add_queries_from_ar(b,*pp_must,BooleanClause::MUST);
            if (pp_must_not)
                add_queries_from_ar(b,*pp_must_not,BooleanClause::MUST_NOT);
            parent->add(b,true,occur);
        } else {
            die("dont know what to do with: %s",key);
        }
    }
    return parent;
}
