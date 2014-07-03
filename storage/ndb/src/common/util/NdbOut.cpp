/* Copyright (c) 2003, 2014, Oracle and/or its affiliates. All rights reserved.


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <ndb_global.h>

#include <NdbOut.hpp>
#include <OutputStream.hpp>

/* Initialized in ndb_init() */
NdbOut ndbout;
NdbOut ndberr;

static const char * fms[] = {
  "%d", "0x%02x",      // Int8
  "%u", "0x%02x",      // Uint8
  "%d", "0x%04x",      // Int16
  "%u", "0x%04x",      // Uint16
  "%d", "0x%08x",      // Int32
  "%u", "0x%08x",      // Uint32
  "%lld", "0x%016llx", // Int64
  "%llu", "0x%016llx", // Uint64
  "%llu", "0x%016llx"  // UintPtr
};

NdbOut& 
NdbOut::operator<<(Int8 v)   { m_out->print(fms[0+isHex],(int)v); return *this;}
NdbOut& 
NdbOut::operator<<(Uint8 v)  { m_out->print(fms[2+isHex],(int)v); return *this;}
NdbOut& 
NdbOut::operator<<(Int16 v)  { m_out->print(fms[4+isHex],(int)v); return *this;}
NdbOut& 
NdbOut::operator<<(Uint16 v) { m_out->print(fms[6+isHex],(int)v); return *this;}
NdbOut& 
NdbOut::operator<<(Int32 v)  { m_out->print(fms[8+isHex], v); return *this;}
NdbOut& 
NdbOut::operator<<(Uint32 v) { m_out->print(fms[10+isHex], v); return *this;}
NdbOut& 
NdbOut::operator<<(Int64 v)  { m_out->print(fms[12+isHex], v); return *this;}
NdbOut& 
NdbOut::operator<<(Uint64 v) { m_out->print(fms[14+isHex], v); return *this;}
NdbOut& 
NdbOut::operator<<(unsigned long int v) { return *this << (Uint64) v; }

NdbOut& 
NdbOut::operator<<(const char* val){ m_out->print("%s", val ? val : "(null)"); return * this; }
NdbOut& 
NdbOut::operator<<(const void* val){ m_out->print("%p", val); return * this; }
NdbOut&
NdbOut::operator<<(BaseString &val){ return *this << val.c_str(); }

NdbOut& 
NdbOut::operator<<(float val){ m_out->print("%f", (double)val); return * this;}
NdbOut& 
NdbOut::operator<<(double val){ m_out->print("%f", val); return * this; }

NdbOut& NdbOut::endline()
{
  isHex = 0; // Reset hex to normal, if user forgot this
  m_out->println("%s", "");
  if (m_autoflush)
    m_out->flush();
  return *this;
}

NdbOut& NdbOut::flushline()
{
  m_out->flush();
  return *this;
}

NdbOut& NdbOut::setHexFormat(int _format)
{
  isHex = (_format == 0 ? 0 : 1);
  return *this;
}

NdbOut& NdbOut::hexdump(const Uint32* words, size_t count)
{
  /**
   * Write at most about 1000 characters.
   * If not all words are printed end with "...\n".
   * Words are written as "H'11223344 ", 11 character each.
   */
  char buf[90 * 11 + 4 + 1];
  size_t offset = 0;
  size_t words_to_dump = count;
  if (words_to_dump > 90)
  {
    words_to_dump = 90;
  }
  for(size_t i = 0 ; i < words_to_dump ; i ++ )
  {
    // Write at most 6 words per line
    char sep = (i % 6 == 5) ? '\n' : ' ';
    assert(offset + 11 < sizeof(buf));
    int n = BaseString::snprintf(buf + offset, sizeof(buf) - offset, "H'%08x%c", words[i], sep);
    assert(n == 11);
    offset += n;
  }
  if (words_to_dump < count)
  {
    assert(offset + 4 < sizeof(buf));
    int n = BaseString::snprintf(buf + offset, sizeof(buf) - offset, "...\n");
    assert(n == 4);
    offset += n;
  }
  else
  {
    assert(offset + 1 < sizeof(buf));
    int n = BaseString::snprintf(buf + offset, sizeof(buf) - offset, "\n");
    assert(n == 1);
    offset += n;
  }
  m_out->write(buf, offset);  
  return *this;
}

NdbOut::NdbOut(OutputStream & out, bool autoflush)
  : m_out(& out), isHex(0), m_autoflush(autoflush)
{
}

NdbOut::NdbOut()
  : m_out(NULL), isHex(0)
{
   /**
    * m_out set to NULL!
    */
}

NdbOut::~NdbOut()
{
   /**
    *  don't delete m_out, as it's a reference given to us.
    *  i.e we don't "own" it
    */
}

void
NdbOut::print(const char * fmt, ...)
{
  if (fmt == NULL)
  {
    /*
     Function was called with fmt being NULL, this is an error
     but handle it gracefully by simpling printing nothing
     instead of continuing down the line whith the NULL pointer.

     Catch problem with an assert in debug compile.
    */
    assert(false);
    return;
  }

  va_list ap;
  char buf[1000];
  
  va_start(ap, fmt);
  BaseString::vsnprintf(buf, sizeof(buf)-1, fmt, ap);
  *this << buf;
  va_end(ap);
}

void
NdbOut::println(const char * fmt, ...)
{
  if (fmt == NULL)
  {
    /*
     Function was called with fmt being NULL, this is an error
     but handle it gracefully by simpling printing nothing
     instead of continuing down the line whith the NULL pointer.

     Catch problem with an assert in debug compile.
    */
    assert(false);
    *this << endl;
    return;
  }

  va_list ap;
  char buf[1000];
  
  va_start(ap, fmt);
  BaseString::vsnprintf(buf, sizeof(buf)-1, fmt, ap);
  *this << buf << endl;
  va_end(ap);
}

static
void 
vndbout_c(const char * fmt, va_list ap)
{
  if (fmt == NULL)
  {
    /*
     Function was called with fmt being NULL, this is an error
     but handle it gracefully by simpling printing an empty newline
     instead of continuing down the line whith the NULL pointer.

     Catch problem with an assert in debug compile.
    */
    assert(false);
    ndbout << endl; // Empty newline
    return;
  }

  char buf[1000];
  BaseString::vsnprintf(buf, sizeof(buf)-1, fmt, ap);
  ndbout << buf << endl;
}

extern "C"
void
ndbout_c(const char * fmt, ...){
  va_list ap;

  va_start(ap, fmt);
  vndbout_c(fmt, ap);
  va_end(ap);
}

extern "C" int ndbout_printer(const char * fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vndbout_c(fmt, ap);
  va_end(ap);
  return 1;
}


FilteredNdbOut::FilteredNdbOut(OutputStream & out, 
			       int threshold, int level)
  : NdbOut(out) {
  m_level = level;
  m_threshold = threshold;
  m_org = &out;
  m_null = new NullOutputStream();
  setLevel(level);
}

FilteredNdbOut::~FilteredNdbOut(){
  delete m_null;
}

void
FilteredNdbOut::setLevel(int i){
  m_level = i;
  if(m_level >= m_threshold){
    m_out = m_org;
  } else {
    m_out = m_null;
  }
}

void
FilteredNdbOut::setThreshold(int i){
  m_threshold = i;
  setLevel(m_level);
}

int
FilteredNdbOut::getLevel() const {
  return m_level;
}
int
FilteredNdbOut::getThreshold() const {
  return m_threshold;
}

static FileOutputStream ndbouts_fileoutputstream(0);
static FileOutputStream ndberrs_fileoutputstream(0);

void
NdbOut_Init()
{
  new (&ndbouts_fileoutputstream) FileOutputStream(stdout);
  new (&ndbout) NdbOut(ndbouts_fileoutputstream);

  new (&ndberrs_fileoutputstream) FileOutputStream(stderr);
  new (&ndberr) NdbOut(ndberrs_fileoutputstream);
}
