#include <hat-trie.h>
#include <ruby.h>
#include <ruby/encoding.h>

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
    bool obj_value;
    HatTrie() {
        obj_value = false;
        p = hattrie_create();
    }
    ~HatTrie() {
        hattrie_free(p);
    }
};

static void hat_mark(void* p_ht) {
    HatTrie* ht = (HatTrie*)p_ht;
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

static VALUE hat_set_type(VALUE self, VALUE is_obj_value) {
    HatTrie* ht;
    Data_Get_Struct(self, HatTrie, ht);
    ht->obj_value = RTEST(is_obj_value);
    return self;
}

static VALUE hat_size(VALUE self) {
    HatTrie* ht;
    Data_Get_Struct(self, HatTrie, ht);
    return ULL2NUM(hattrie_size(ht->p));
}

static VALUE hat_set(VALUE self, VALUE key, VALUE value) {
    PRE_HAT;
    hattrie_get(p, RSTRING_PTR(key), RSTRING_LEN(key))[0] = ht->obj_value ? value : NUM2LL(value);
    return self;
}

static VALUE hat_get(VALUE self, VALUE key) {
    PRE_HAT;
    value_t* vt = hattrie_tryget(p, RSTRING_PTR(key), RSTRING_LEN(key));
    if (vt) {
        return ht->obj_value ? (*vt) : LL2NUM(*vt);
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
        return ht->obj_value ? (*vt) : LL2NUM(*vt);
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
        if (vlimit != Qnil && limit-- <= 0) {
            break;
        }
        size_t suffix_len;
        const char* suffix_s = hattrie_iter_key(it, &suffix_len);
        value_t* v = hattrie_iter_val(it);
        volatile VALUE suffix = rb_enc_str_new(suffix_s, suffix_len, u8_enc);
        volatile VALUE value = ht->obj_value ? (*v) : LL2NUM(*v);
        rb_funcall(callback, rb_intern("call"), 2, suffix, value);
        hattrie_iter_next(it);
        limit--;
    }
    hattrie_iter_free(it); // todo re-raise on error
    return Qnil;
}

#define DEF(k,n,f,c) rb_define_method(k,n,RUBY_METHOD_FUNC(f),c)

extern "C"
void Init_triez() {
    hat_class = rb_define_class("Triez", rb_cObject);
    u8_enc = rb_utf8_encoding();
    bin_enc = rb_ascii8bit_encoding();

    rb_define_alloc_func(hat_class, hat_alloc);
    DEF(hat_class, "_internal_set_type", hat_set_type, 1);
    DEF(hat_class, "size", hat_size, 0);
    DEF(hat_class, "has_key?", hat_check, 1);
    DEF(hat_class, "[]=", hat_set, 2);
    DEF(hat_class, "[]", hat_get, 1);
    DEF(hat_class, "delete", hat_del, 1);
    DEF(hat_class, "_internal_search", hat_search, 3);
}
