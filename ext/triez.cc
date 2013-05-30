#include <hat-trie.h>
#include <ruby.h>
#include <ruby/encoding.h>

// for rubinius
#ifndef rb_enc_fast_mbclen
#   define rb_enc_fast_mbclen rb_enc_mbclen
#endif

static VALUE hat_class;
static rb_encoding* u8_enc;
static rb_encoding* bin_enc;

static inline VALUE unify_key(VALUE key) {
    rb_encoding* enc = rb_enc_get(key);
    if (enc != u8_enc && enc != bin_enc) {
        return rb_funcall(key, rb_intern("encode"), 1, rb_enc_from_encoding(u8_enc));
    } else {
        return key;
    }
}

static inline long long V2LL(VALUE v) {
    union {VALUE v; long long l;} u;
    u.v = v;
    return u.l;
}

static inline VALUE LL2V(long long l) {
    union {VALUE v; long long l;} u;
    u.l = l;
    return u.v;
}

struct HatTrie {
    hattrie_t* p;
    VALUE default_value;
    bool obj_value;
    bool initialized;

    HatTrie() : default_value(Qnil), obj_value(false), initialized(false) {
        p = hattrie_create();
    }

    ~HatTrie() {
        hattrie_free(p);
    }
};

static void hat_mark(void* p_ht) {
    HatTrie* ht = (HatTrie*)p_ht;
    if (!IMMEDIATE_P(ht->default_value)) {
        rb_gc_mark(ht->default_value);
    }
    if (!ht->obj_value) {
        return;
    }
    hattrie_t* p = ht->p;
    hattrie_iter_t* it = hattrie_iter_begin(p, false);
    while (!hattrie_iter_finished(it)) {
        value_t* v = hattrie_iter_val(it);
        if (!IMMEDIATE_P(*v)) {
            rb_gc_mark(*v);
        }
    }
    hattrie_iter_free(it);
}

static void hat_free(void* p) {
    delete (HatTrie*)p;
}

static VALUE hat_alloc(VALUE self) {
    HatTrie* ht = new HatTrie();
    return Data_Wrap_Struct(hat_class, hat_mark, hat_free, ht);
}

#define PRE_HAT\
    hattrie_t* p;\
    HatTrie* ht;\
    Data_Get_Struct(self, HatTrie, ht);\
    p = ht->p;\
    Check_Type(key, T_STRING);\
    key = unify_key(key);

static VALUE hat_set_type(VALUE self, VALUE obj_value, VALUE default_value) {
    HatTrie* ht;
    Data_Get_Struct(self, HatTrie, ht);
    if (ht->initialized) {
        rb_raise(rb_eRuntimeError, "Already initialized");
        return self;
    }
    ht->default_value = default_value;
    ht->obj_value = RTEST(obj_value);
    ht->initialized = true;
    return self;
}

static VALUE hat_value_type(VALUE self) {
    HatTrie* ht;
    Data_Get_Struct(self, HatTrie, ht);
    return ht->obj_value ? ID2SYM(rb_intern("object")) : ID2SYM(rb_intern("int64"));
}

static VALUE hat_size(VALUE self) {
    HatTrie* ht;
    Data_Get_Struct(self, HatTrie, ht);
    return ULL2NUM(hattrie_size(ht->p));
}

static VALUE hat_set(VALUE self, VALUE key, VALUE value) {
    PRE_HAT;
    long long v = ht->obj_value ? value : NUM2LL(value);
    char* s = RSTRING_PTR(key);
    size_t len = RSTRING_LEN(key);
    hattrie_get(p, s, len)[0] = v;
    return self;
}

static inline void hat_change(HatTrie* ht, hattrie_t* p, char* s, size_t len) {
    // NOTE must use 2-step change, because the block may change the trie
    value_t* vp = hattrie_tryget(p, s, len);
    long long v;
    if (ht->obj_value) {
        VALUE value = vp ? LL2V(vp[0]) : ht->default_value;
        v = V2LL(rb_yield(value));
    } else {
        VALUE value = vp ? LL2NUM(vp[0]) : ht->default_value;
        v = NUM2LL(rb_yield(value));
    }
    hattrie_get(p, s, len)[0] = v;
}

static inline void hat_change_prefix(HatTrie* ht, hattrie_t* p, char* s, size_t len, char* rs) {
    char* rs_end = rs + len;
    long n;
    for (; rs < rs_end; rs += n, len -= n) {
        hat_change(ht, p, s, len);
        // no need check encoding because reverse succeeded
        n = rb_enc_fast_mbclen(rs, rs_end, u8_enc);
    }
}

static VALUE hat_change_all(VALUE self, VALUE type, VALUE key) {
    PRE_HAT;
    char* s = RSTRING_PTR(key);
    size_t len = RSTRING_LEN(key);
    ID ty = SYM2ID(type);
    if (ty == rb_intern("suffix")) {
        char* s_end = s + len;
        long n;
        for (; s < s_end; s += n, len -= n) {
            hat_change(ht, p, s, len);
            n = rb_enc_mbclen(s, s_end, u8_enc);
        }
    } else if (ty == rb_intern("prefix")) {
        volatile VALUE reversed = rb_funcall(key, rb_intern("reverse"), 0);
        hat_change_prefix(ht, p, s, len, RSTRING_PTR(reversed));
    } else if (ty == rb_intern("substring")) {
        volatile VALUE reversed = rb_funcall(key, rb_intern("reverse"), 0);
        char* rs = RSTRING_PTR(reversed);
        char* s_end = s + len;
        long n;
        for (; s < s_end; s += n, len -= n) {
            hat_change_prefix(ht, p, s, len, rs);
            n = rb_enc_fast_mbclen(s, s_end, u8_enc);
        }
    }
    return self;
}

static VALUE hat_append(VALUE self, VALUE key) {
    HatTrie* ht;
    Data_Get_Struct(self, HatTrie, ht);
    return hat_set(self, key, ht->default_value);
}

static VALUE hat_get(VALUE self, VALUE key) {
    PRE_HAT;
    value_t* vt = hattrie_tryget(p, RSTRING_PTR(key), RSTRING_LEN(key));
    if (vt) {
        return ht->obj_value ? (*vt) : LL2NUM(*vt);
    } else {
        return ht->default_value;
    }
}

static VALUE hat_del(VALUE self, VALUE key) {
    PRE_HAT;
    const char* s = RSTRING_PTR(key);
    size_t len = RSTRING_LEN(key);
    value_t* vt = hattrie_tryget(p, s, len);
    if (vt) {
        hattrie_del(p, RSTRING_PTR(key), RSTRING_LEN(key));
        return ht->obj_value ? (*vt) : LL2NUM(*vt);
    } else {
        return ht->default_value;
    }
}

static VALUE hat_check(VALUE self, VALUE key) {
    PRE_HAT;
    value_t* vt = hattrie_tryget(p, RSTRING_PTR(key), RSTRING_LEN(key));
    return vt ? Qtrue : Qfalse;
}

struct SearchCbData {
    VALUE callback;
    VALUE suffix;
    VALUE value;
};

static VALUE hat_search_callback(VALUE data) {
    SearchCbData* p = (SearchCbData*)data;
    return rb_funcall(p->callback, rb_intern("call"), 2, p->suffix, p->value);
}

static VALUE hat_search(VALUE self, VALUE key, VALUE vlimit, VALUE vsort, VALUE callback) {
    PRE_HAT;
    long limit = 0;
    if (vlimit != Qnil) {
        limit = NUM2LONG(vlimit);
    }

    hattrie_iter_t* it = hattrie_iter_with_prefix(p, RTEST(vsort), RSTRING_PTR(key), RSTRING_LEN(key));
    int error = 0;
    SearchCbData data = {callback};
    while (!hattrie_iter_finished(it)) {
        if (vlimit != Qnil && limit-- <= 0) {
            break;
        }
        size_t suffix_len;
        const char* suffix_s = hattrie_iter_key(it, &suffix_len);
        value_t* v = hattrie_iter_val(it);
        data.suffix = rb_enc_str_new(suffix_s, suffix_len, u8_enc);
        data.value = ht->obj_value ? (*v) : LL2NUM(*v);
        rb_protect(hat_search_callback, (VALUE)&data, &error);
        if (error) {
            break;
        }
        hattrie_iter_next(it);
    }
    hattrie_iter_free(it);
    if (error) {
        rb_funcall(rb_mKernel, rb_intern("raise"), 0);
    }
    return self;
}

VALUE hat_longest_match(VALUE self, VALUE key) {
    PRE_HAT;
    size_t len = (size_t)RSTRING_LEN(key);
    value_t* vt = hattrie_tryget_longest_match(p, RSTRING_PTR(key), &len);
    if (vt) {
        volatile VALUE r = rb_ary_new();
        rb_ary_push(r, rb_funcall(key, rb_intern("byteslice"), 2, INT2FIX(0), ULONG2NUM(len)));
        rb_ary_push(r, (ht->obj_value ? (*vt) : LL2NUM(*vt)));
        return r;
    } else {
        return Qnil;
    }
}

#define DEF(k,n,f,c) rb_define_method(k,n,RUBY_METHOD_FUNC(f),c)

extern "C"
void Init_triez() {
    hat_class = rb_define_class("Triez", rb_cObject);
    u8_enc = rb_utf8_encoding();
    bin_enc = rb_ascii8bit_encoding();

    rb_define_alloc_func(hat_class, hat_alloc);
    DEF(hat_class, "_internal_set_type", hat_set_type, 2);
    DEF(hat_class, "value_type", hat_value_type, 0);
    DEF(hat_class, "size", hat_size, 0);
    DEF(hat_class, "[]=", hat_set, 2);
    DEF(hat_class, "change_all", hat_change_all, 2);
    DEF(hat_class, "<<", hat_append, 1);
    DEF(hat_class, "[]", hat_get, 1);
    DEF(hat_class, "has_key?", hat_check, 1);
    DEF(hat_class, "delete", hat_del, 1);
    DEF(hat_class, "_internal_search", hat_search, 4);
    DEF(hat_class, "longest_match", hat_longest_match, 1);
}
