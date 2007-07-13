/** @file matchspy.cc
 * @brief MatchDecider subclasses for use as "match spies".
 */
/* Copyright (C) 2007 Olly Betts
 * Copyright (C) 2007 Lemur Consulting Ltd
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

#include <xapian/document.h>
#include <xapian/matchspy.h>
#include <xapian/queryparser.h>

#include <float.h>
#include <math.h>

#include <algorithm>
#include <map>
#include <vector>
#include <string>

#include "omassert.h"
#include "stringutils.h"

using namespace std;

namespace Xapian {

bool
ValueCountMatchSpy::operator()(const Document &doc) const
{
    ++total;
    map<Xapian::valueno, map<string, size_t> >::iterator i;
    for (i = values.begin(); i != values.end(); ++i) {
	Xapian::valueno valno = i->first;
	map<string, size_t> & tally = i->second;

	string val(doc.get_value(valno));
	if (!val.empty()) ++tally[val];
    }
    return true;
}

bool
TermCountMatchSpy::operator()(const Document &doc) const
{
    ++documents_seen;
    map<std::string, map<string, size_t> >::iterator i;
    for (i = terms.begin(); i != terms.end(); ++i) {
	std::string prefix = i->first;
	map<string, size_t> & tally = i->second;

	TermIterator j = doc.termlist_begin();
	j.skip_to(prefix);
	for (; j != doc.termlist_end() && startswith((*j), prefix); ++j) {
	    ++tally[(*j).substr(prefix.size())];
	    ++terms_seen;
	}
    }
    return true;
}

inline double sqrd(double x) { return x * x; }

double
CategorySelectMatchSpy::score_categorisation(Xapian::valueno valno,
					     double desired_no_of_categories)
{
    if (total == 0) return 0.0;

    const map<string, size_t> & cat = values[valno];
    size_t total_unset = total;
    double score = 0.0;

    if (desired_no_of_categories <= 0.0)
	desired_no_of_categories = cat.size();

    double avg = double(total) / desired_no_of_categories;

    map<string, size_t>::const_iterator i;
    for (i = cat.begin(); i != cat.end(); ++i) {
	size_t count = i->second;
	total_unset -= count;
	score += sqrd(count - avg);
    }
    if (total_unset) score += sqrd(total_unset - avg);

    // Scale down so the total number of items doesn't make a difference.
    score /= sqrd(total);

    // Bias towards returning the number of categories requested.
    score += 0.01 * sqrd(desired_no_of_categories - cat.size());

    return score;
}

struct bucketval {
    size_t count;
    double min, max;

    bucketval() : count(0), min(DBL_MAX), max(-DBL_MAX) { }

    void update(size_t n, double value) {
	count += n;
	if (value < min) min = value;
	if (value > max) max = value;
    }
};

bool
CategorySelectMatchSpy::build_numeric_ranges(Xapian::valueno valno, size_t max_ranges)
{
    const map<string, size_t> & cat = values[valno];

    double lo = DBL_MAX, hi = -DBL_MAX;

    map<double, size_t> histo;
    size_t total_set = 0;
    map<string, size_t>::const_iterator i;
    for (i = cat.begin(); i != cat.end(); ++i) {
	double v = Xapian::sortable_unserialise(i->first.c_str());
	if (v < lo) lo = v;
	if (v > hi) hi = v;
	size_t count = i->second;
	histo[v] = count;
	total_set += count;
    }

    if (total_set == 0) {
	// No set values.
	return false;
    }
    if (lo == hi) {
	// All set values are the same.
	return false;
    }

    double sizeby = max(fabs(hi), fabs(lo));
    // E.g. if sizeby = 27.4 and max_ranges = 7, we want to split into units of
    // width 1.0 which we may then coalesce if there are too many used buckets.
    double unit = pow(10.0, floor(log10(sizeby / max_ranges) - 0.2));
    double start = floor(lo / unit) * unit;
    size_t n_buckets = size_t(ceil(hi / unit) - floor(lo / unit));

    bool scaleby2 = true;
    vector<bucketval> bucket(n_buckets + 1);
    while (true) {
	size_t n_used = 0;
	map<double, size_t>::const_iterator j;
	for (j = histo.begin(); j != histo.end(); ++j) {
	    double v = j->first;
	    size_t b = size_t(floor((v - start) / unit));
	    if (bucket[b].count == 0) ++n_used;
	    bucket[b].update(j->second, v);
	}

	if (n_used <= max_ranges) break;

	unit *= scaleby2 ? 2.0 : 2.5;
	scaleby2 = !scaleby2;
	start = floor(lo / unit) * unit;
	n_buckets = size_t(ceil(hi / unit) - floor(lo / unit));
	bucket.resize(0);
	bucket.resize(n_buckets + 1);
    }

    map<string, size_t> discrete_categories;
    for (size_t b = 0; b < bucket.size(); ++b) {
	if (bucket[b].count == 0) continue;
	string encoding = Xapian::sortable_serialise(bucket[b].min);
	if (bucket[b].min != bucket[b].max) {
	    // Pad the start to 9 bytes with zeros.
	    encoding.resize(9);
	    encoding += Xapian::sortable_serialise(bucket[b].max);
	}
	discrete_categories[encoding] = bucket[b].count;
    }

    size_t total_unset = total - total_set;
    if (total_unset) {
	discrete_categories[""] = total_unset;
    }

    swap(discrete_categories, values[valno]);

    return true;
}

class StringAndFreqCmpByFreq {
  public:
    StringAndFreqCmpByFreq() {}

    // Return true if a has a lower frequency than b.
    // If equal, compare by the str, to provide a stable sort order.
    bool operator()(const StringAndFrequency &a,
		    const StringAndFrequency &b) const {
	if (a.frequency > b.frequency) return true;
	if (a.frequency < b.frequency) return false;
	if (a.str > b.str) return false;
	return true;
    }
};

void
get_most_frequent_items(vector<StringAndFrequency> & result,
			const map<string, size_t> & items,
			size_t maxitems)
{
    result.clear();
    result.reserve(maxitems);
    StringAndFreqCmpByFreq cmpfn;
    bool is_heap(false);

    for (map<string, size_t>::const_iterator i = items.begin();
	 i != items.end(); i++) {
	Assert(result.size() <= maxitems);
	result.push_back(StringAndFrequency(i->first, i->second));
	if (result.size() > maxitems) {
	    // Make the list back into a heap.
	    if (is_heap) {
		// Only the new element isn't in the right place.
		push_heap(result.begin(), result.end(), cmpfn);
	    } else {
		// Need to build heap from scratch.
		make_heap(result.begin(), result.end(), cmpfn);
		is_heap = true;
	    }
	    pop_heap(result.begin(), result.end(), cmpfn);
	    result.pop_back();
	}
    }

    if (is_heap) {
	sort_heap(result.begin(), result.end(), cmpfn);
    } else {
	sort(result.begin(), result.end(), cmpfn);
    }
}

}
