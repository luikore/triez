#include <marisa.h>
#include <hat-trie.h>
#include <ruby.h>
#include <ruby/encoding.h>

static VALUE hat_class;
static VALUE valued_hat_class;
static VALUE marisa_class;
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

static void hat_free(void* p) {
    hattrie_free((hattrie_t*)p);
}

static VALUE hat_alloc(VALUE self) {
    hattrie_t* p = hattrie_create();
    return Data_Wrap_Struct(hat_class, NULL, hat_free, p);
}

#define PRE_HAT\
    hattrie_t* p;\
    Data_Get_Struct(self, hattrie_t, p);\
    Check_Type(key, T_STRING);\
    key = unify_key(key);

static VALUE hat_set(VALUE self, VALUE key, VALUE value) {
    PRE_HAT;
    hattrie_get(p, RSTRING_PTR(key), RSTRING_LEN(key))[0] = NUM2LL(value);
    return self;
}

static VALUE hat_get(VALUE self, VALUE key) {
    PRE_HAT;
    value_t* vt = hattrie_tryget(p, RSTRING_PTR(key), RSTRING_LEN(key));
    if (vt) {
        return LL2NUM(*vt);
    } else {
        return Qnil;
    }
}

static VALUE hat_del(VALUE self, VALUE key) {
    PRE_HAT;
    const char* s = RSTRING_PTR(key);
    size_t len = RSTRING_LEN(key);
    value_t* vt = hattrie_tryget(p, s, len);
    if (vt) {
        hattrie_del(p, RSTRING_PTR(key), RSTRING_LEN(key));
        return LL2NUM(*vt);
    } else {
        return Qnil;
    }
}

static VALUE hat_check(VALUE self, VALUE key) {
    PRE_HAT;
    value_t* vt = hattrie_tryget(p, RSTRING_PTR(key), RSTRING_LEN(key));
    return vt ? Qtrue : Qfalse;
}

static VALUE hat_search(VALUE self, VALUE key, VALUE vlimit, VALUE callback) {
    PRE_HAT;
    long limit = 0;
    if (vlimit != Qnil) {
        limit = NUM2LONG(vlimit);
    }

    hattrie_iter_t* it = hattrie_iter_with_prefix(p, false, RSTRING_PTR(key), RSTRING_LEN(key));
    while (!hattrie_iter_finished(it)) {
        size_t suffix_len;
        const char* suffix_s = hattrie_iter_key(it, &suffix_len);
        value_t* v = hattrie_iter_val(it);
        VALUE suffix = rb_enc_str_new(suffix_s, suffix_len, u8_enc);
        rb_funcall(callback, rb_intern("call"), 2, suffix, LL2NUM(*v));
        hattrie_iter_next(it);
    }
    hattrie_iter_free(it); // todo re-raise on error
    return Qnil;
}

static void valued_hat_mark(void* p) {
    hattrie_t* t = (hattrie_t*)p;
    /*
    rb_gc_mark(v);
    */
}

static VALUE valued_hat_alloc(VALUE self) {
    hattrie_t* p = hattrie_create();
    return Data_Wrap_Struct(valued_hat_class, valued_hat_mark, hat_free, p);
}

// valued_hat_check is the same as hat_check

static VALUE valued_hat_set(VALUE self, VALUE key, VALUE value) {
    PRE_HAT;
    hattrie_get(p, RSTRING_PTR(key), RSTRING_LEN(key))[0] = V2LL(value);
    return self;
}

static VALUE valued_hat_get(VALUE self, VALUE key) {
    PRE_HAT;
    value_t* vt = hattrie_tryget(p, RSTRING_PTR(key), RSTRING_LEN(key));
    if (vt) {
        return LL2V(*vt);
    } else {
        return Qnil;
    }
}

static VALUE valued_hat_del(VALUE self, VALUE key) {
    PRE_HAT;
    const char* s = RSTRING_PTR(key);
    size_t len = RSTRING_LEN(key);
    value_t* vt = hattrie_tryget(p, s, len);
    if (vt) {
        hattrie_del(p, RSTRING_PTR(key), RSTRING_LEN(key));
        return LL2V(*vt);
    } else {
        return Qnil;
    }
}

static VALUE marisa_alloc(VALUE self) {

    return Qnil;
}

#define DEF(k,n,f,c) rb_define_method(k,n,RUBY_METHOD_FUNC(f),c)

extern "C"
void Init_triez() {
    VALUE triez = rb_define_module("Triez");
    u8_enc = rb_utf8_encoding();
    bin_enc = rb_ascii8bit_encoding();

    hat_class = rb_define_class_under(triez, "HatTrie", rb_cObject);
    rb_define_alloc_func(hat_class, hat_alloc);
    rb_define_singleton_method(triez, "hat", RUBY_METHOD_FUNC(hat_alloc), 0);
    DEF(hat_class, "[]=", hat_set, 2);
    DEF(hat_class, "[]", hat_get, 1);
    DEF(hat_class, "delete", hat_del, 1);
    DEF(hat_class, "has_key?", hat_check, 1);
    DEF(hat_class, "_internal_search", hat_search, 3);

    valued_hat_class = rb_define_class_under(triez, "ValuedHatTrie", hat_class);
    rb_define_alloc_func(valued_hat_class, valued_hat_alloc);
    rb_define_singleton_method(triez, "valued_hat", RUBY_METHOD_FUNC(valued_hat_alloc), 0);
    DEF(valued_hat_class, "[]=", valued_hat_set, 2);
    DEF(valued_hat_class, "[]", valued_hat_get, 1);
    DEF(valued_hat_class, "delete", valued_hat_del, 1);

    marisa_class = rb_define_class_under(triez, "MarisaTrie", rb_cObject);
    rb_define_alloc_func(marisa_class, marisa_alloc);
    rb_define_singleton_method(triez, "marisa", RUBY_METHOD_FUNC(marisa_alloc), 0);
}
