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

static void hat_free(void* p) {
	hattrie_free((hattrie_t*)p);
}

static VALUE hat_alloc(VALUE self) {
	hattrie_t* p = hattrie_create();
	return Data_Wrap_Struct(hat_class, NULL, hat_free, p);
}

static VALUE hat_set(VALUE self, VALUE key, VALUE value) {
    hattrie_t* p;
    Data_Get_Struct(self, hattrie_t, p);

    key = unify_key(key);
    hattrie_get(p, RSTRING_PTR(key), RSTRING_LEN(key))[0] = NUM2LL(value);
    return self;
}

static VALUE hat_get(VALUE self, VALUE key) {
    hattrie_t* p;
    Data_Get_Struct(self, hattrie_t, p);

    key = unify_key(key);
    value_t* vt = hattrie_tryget(p, RSTRING_PTR(key), RSTRING_LEN(key));
    if (vt) {
        return LL2NUM(*vt);
    } else {
        return Qnil;
    }
}

static VALUE hat_check(VALUE self, VALUE key) {
    hattrie_t* p;
    Data_Get_Struct(self, hattrie_t, p);

    key = unify_key(key);
    value_t* vt = hattrie_tryget(p, RSTRING_PTR(key), RSTRING_LEN(key));
    return vt ? Qtrue : Qfalse;
}

static VALUE hat_del(VALUE self, VALUE key) {
    hattrie_t* p;
    Data_Get_Struct(self, hattrie_t, p);

    key = unify_key(key);
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

static VALUE hat_iter(VALUE self) {

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

	valued_hat_class = rb_define_class_under(triez, "ValuedHatTrie", hat_class);
	rb_define_alloc_func(valued_hat_class, valued_hat_alloc);
	rb_define_singleton_method(triez, "valued_hat", RUBY_METHOD_FUNC(valued_hat_alloc), 0);

	marisa_class = rb_define_class_under(triez, "MarisaTrie", rb_cObject);
	rb_define_alloc_func(marisa_class, marisa_alloc);
	rb_define_singleton_method(triez, "marisa", RUBY_METHOD_FUNC(marisa_alloc), 0);
}
