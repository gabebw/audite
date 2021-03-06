#include <ruby.h>

#include <stdio.h>
#include <strings.h>
#include <mpg123.h>

void cleanup(mpg123_handle *mh)
{
  mpg123_close(mh);
  mpg123_delete(mh);
}

static VALUE rb_cMpg123;

VALUE rb_mpg123_new(VALUE klass, VALUE filename) {
  int err = MPG123_OK;
  mpg123_handle *mh;
  VALUE mpg123;
  long rate;
  int channels, encoding;

  Check_Type(filename, T_STRING);

  if ((mh = mpg123_new(NULL, &err)) == NULL) {
    rb_raise(rb_eStandardError, "%s", mpg123_plain_strerror(err));
  }

  mpg123_param(mh, MPG123_ADD_FLAGS, MPG123_FORCE_FLOAT, 0.);

  if (mpg123_open(mh, (char*) RSTRING_PTR(filename)) != MPG123_OK ||
      mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {
    rb_raise(rb_eStandardError, "%s", mpg123_strerror(mh));
  }

  if (encoding != MPG123_ENC_FLOAT_32) {
    rb_raise(rb_eStandardError, "bad encoding");
  }

  mpg123_format_none(mh);
  mpg123_format(mh, rate, channels, encoding);

  VALUE new_mpg123 = Data_Wrap_Struct(rb_cMpg123, 0, cleanup, mh);
  rb_iv_set(new_mpg123, "@file", filename);
  return new_mpg123;
}

static VALUE rb_mpg123_file(VALUE self, VALUE obj)
{
  return rb_iv_get(self, "@file");
}

VALUE rb_mpg123_close(VALUE self)
{
  mpg123_close(DATA_PTR(self));
  return self;
}

VALUE rb_mpg123_read(VALUE self, VALUE _size)
{
  int size = FIX2INT(_size);
  float *buffer = malloc(size * sizeof(float));
  VALUE result = rb_ary_new2(size);
  mpg123_handle *mh = NULL;
  size_t done = 0;
  int i;
  int err = MPG123_OK;

  Data_Get_Struct(self, mpg123_handle, mh);
  err = mpg123_read(mh, (unsigned char *) buffer, size * sizeof(float), &done);

  if (err == MPG123_OK || err == MPG123_DONE) {
    for (i = 0; i < size; i++) {
      rb_ary_store(result, i, rb_float_new(buffer[i]));
    }
    free(buffer);
    return result;
  }
  else {
    free(buffer);
    rb_raise(rb_eStandardError, "%s", mpg123_plain_strerror(err));
  }
}

VALUE rb_mpg123_length(VALUE self)
{
  /*
   * mpg123_length() only returns an estimated duration
   * if the song hasn't previously been scanned.
   * This can be incorrect if, for example, the song is corrupted
   * and cannot be played after a certain point.
   * Run mpg123_scan() first to get an accurate length reading.
   */
  mpg123_scan(DATA_PTR(self));
  return INT2FIX(mpg123_length(DATA_PTR(self)));
}

VALUE rb_mpg123_spf(VALUE self)
{
  return rb_float_new(mpg123_spf(DATA_PTR(self)));
}

VALUE rb_mpg123_tpf(VALUE self)
{
  return rb_float_new(mpg123_tpf(DATA_PTR(self)));
}

VALUE rb_mpg123_tell(VALUE self)
{
  return INT2FIX(mpg123_tell(DATA_PTR(self)));
}

VALUE rb_mpg123_tellframe(VALUE self)
{
  return INT2FIX(mpg123_tellframe(DATA_PTR(self)));
}

VALUE rb_mpg123_seek(VALUE self, VALUE offset)
{
  return INT2FIX(mpg123_seek(DATA_PTR(self), FIX2INT(offset), SEEK_SET));
}

VALUE rb_mpg123_seek_frame(VALUE self, VALUE offset)
{
  return INT2FIX(mpg123_seek_frame(DATA_PTR(self), FIX2INT(offset), SEEK_SET));
}

VALUE rb_mpg123_timeframe(VALUE self, VALUE seconds)
{
  return INT2FIX(mpg123_timeframe(DATA_PTR(self), NUM2DBL(seconds)));
}

void Init_mpg123(void) {
  int err = MPG123_OK;

  err = mpg123_init();

  if (err != MPG123_OK) {
    rb_raise(rb_eStandardError, "%s", mpg123_plain_strerror(err));
  }

  rb_cMpg123 = rb_define_class("Mpg123", rb_cObject);

  rb_define_singleton_method(rb_cMpg123, "new", rb_mpg123_new, 1);

  rb_define_method(rb_cMpg123, "file", rb_mpg123_file, 0);

  rb_define_method(rb_cMpg123, "close", rb_mpg123_close, 0);
  rb_define_method(rb_cMpg123, "read", rb_mpg123_read, 1);
  rb_define_method(rb_cMpg123, "length", rb_mpg123_length, 0);
  rb_define_method(rb_cMpg123, "spf", rb_mpg123_spf, 0);
  rb_define_method(rb_cMpg123, "tpf", rb_mpg123_tpf, 0);
  rb_define_method(rb_cMpg123, "tell", rb_mpg123_tell, 0);
  rb_define_method(rb_cMpg123, "tellframe", rb_mpg123_tellframe, 0);
  rb_define_method(rb_cMpg123, "seek", rb_mpg123_seek, 1);
  rb_define_method(rb_cMpg123, "seek_frame", rb_mpg123_seek_frame, 1);
  rb_define_method(rb_cMpg123, "timeframe", rb_mpg123_timeframe, 1);
}
