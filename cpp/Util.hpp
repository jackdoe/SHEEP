// from http://search.cpan.org/~tbusch/Lucene-0.01/lib/Lucene.pm

wchar_t* CharToWChar(int is_utf8, U8 *src, STRLEN arg_len) {
    wchar_t* ret;
    // Alloc memory for wide char string.  This could be a bit more
    // then necessary.
    Newz(0, ret, arg_len + 1, wchar_t);
    wchar_t* dst = ret;

    if (is_utf8) {
        // UTF8 to wide char mapping
        STRLEN len;
        while (*src) {
            *dst++ = utf8_to_uvuni(src, &len);
            src += len;
        }
    } else {
        // char to wide char mapping
        while (*src) {
            *dst++ = (wchar_t) *src++;
        }
    }
    *dst = 0;
    return ret;
}

wchar_t *SvToWChar(SV* arg) {
    wchar_t* ret;
    STRLEN arg_len;
    SvPV(arg, arg_len);
    return CharToWChar(SvUTF8(arg) ? 1 : 0,(U8 *) SvPV_nolen(arg),arg_len);
}

SV* WCharToSv(const wchar_t* src, SV* dest) {
    U8* dst;
    U8* d;

    // Alloc memory for wide char string.  This is clearly wider
    // then necessary in most cases but no choice.
    Newz(0, dst, 3 * wcslen(src) + 1, U8);

    d = dst;
    while (*src) {
        d = uvuni_to_utf8(d, *src++);
    }
    *d = 0;

    sv_setpv(dest, (char*) dst);
    sv_utf8_decode(dest);

    Safefree(dst);
    return dest;
}
