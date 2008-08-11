/** @file chert_termlisttable.cc
 * @brief Subclass of ChertTable which holds termlists.
 */
/* Copyright (C) 2007,2008 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <config.h>

#include "chert_termlisttable.h"

#include <xapian/document.h>
#include <xapian/error.h>
#include <xapian/termiterator.h>

#include "chert_utils.h"
#include "omassert.h"
#include "omdebug.h"
#include "stringutils.h"
#include "utils.h"

#include <string>

using namespace std;

void
ChertTermListTable::set_termlist(Xapian::docid did,
				 const Xapian::Document & doc,
				 chert_doclen_t doclen)
{
    DEBUGCALL(DB, void, "ChertTermListTable::set_termlist",
	      did << ", " << doc << ", " << doclen);

    string tag = pack_uint(doclen);

    Xapian::doccount termlist_size = doc.termlist_count();
    if (termlist_size == 0) {
	// doclen is sum(wdf) so should be zero if there are no terms.
	Assert(doclen == 0);
	Assert(doc.termlist_begin() == doc.termlist_end());
	add(chert_docid_to_key(did), string());
	return;
    }

    Xapian::TermIterator t = doc.termlist_begin();
    if (t != doc.termlist_end()) {
	tag += pack_uint(termlist_size);
	string prev_term = *t;

	tag += prev_term.size();
	tag += prev_term;
	tag += pack_uint(t.get_wdf());
	--termlist_size;

	while (++t != doc.termlist_end()) {
	    const string & term = *t;
	    // If there's a shared prefix with the previous term, we don't
	    // store it explicitly, but just store the length of the shared
	    // prefix.  In general, this is a big win.
	    size_t reuse = common_prefix_length(prev_term, term);

	    // reuse must be <= prev_term.size(), and we know that value while
	    // decoding.  So if the wdf is small enough that we can multiply it
	    // by (prev_term.size() + 1), add reuse and fit the result in a
	    // byte, then we can pack reuse and the wdf into a single byte and
	    // save ourselves a byte.  We actually need to add one to the wdf
	    // before multiplying so that a wdf of 0 can be detected by the
	    // decoder.
	    size_t packed = 0;
	    Xapian::termcount wdf = t.get_wdf();
	    // If wdf >= 128, then we aren't going to be able to pack it in so
	    // don't even try to avoid the calculation overflowing and making
	    // us think we can.
	    if (wdf < 127)
		packed = (wdf + 1) * (prev_term.size() + 1) + reuse;

	    if (packed && packed < 256) {
		// We can pack the wdf into the same byte.
		tag += char(packed);
		tag += char(term.size() - reuse);
		tag.append(term.data() + reuse, term.size() - reuse);
	    } else {
		tag += char(reuse);
		tag += char(term.size() - reuse);
		tag.append(term.data() + reuse, term.size() - reuse);
		// FIXME: pack wdf after reuse next time we rejig the format
		// incompatibly.
		tag += pack_uint(wdf);
	    }

	    prev_term = *t;
	    --termlist_size;
	}
    }
    Assert(termlist_size == 0);
    add(chert_docid_to_key(did), tag);
}

chert_doclen_t
ChertTermListTable::get_doclength(Xapian::docid did) const
{
    DEBUGCALL(DB, chert_doclen_t, "ChertTermListTable::get_doclength", did);

    string tag;
    if (!get_exact_entry(chert_docid_to_key(did), tag))
	throw Xapian::DocNotFoundError("No termlist found for document " +
				       om_tostring(did));

    if (tag.empty()) RETURN(0);

    const char * pos = tag.data();
    const char * end = pos + tag.size();

    chert_doclen_t doclen;
    if (!unpack_uint(&pos, end, &doclen)) {
	const char *msg;
	if (pos == 0) {
	    msg = "Too little data for doclen in termlist";
	} else {
	    msg = "Overflowed value for doclen in termlist";
	}
	throw Xapian::DatabaseCorruptError(msg);
    }

    RETURN(doclen);
}
