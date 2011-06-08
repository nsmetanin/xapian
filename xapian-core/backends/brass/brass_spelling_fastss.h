/** @file brass_spelling_fastss.h
 * @brief FastSS spelling correction algorithm for a brass database.
 */
/* Copyright (C) 2011 Nikita Smetanin
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

#ifndef XAPIAN_INCLUDED_BRASS_SPELLING_FASTSS_H
#define XAPIAN_INCLUDED_BRASS_SPELLING_FASTSS_H

#include <xapian/types.h>

#include "brass_spelling.h"
#include "termlist.h"

#include <vector>
#include <xapian/unordered_set.h>

#include <string>

class BrassSpellingTableFastSS : public BrassSpellingTable
{
		static const unsigned MAX_DISTANCE = 2;
		static const unsigned LIMIT = 8; //There is strange behavior when LIMIT > 8
		static const unsigned PREFIX_LENGTH = 4;

		class TermDataCompare
		{
				const BrassSpellingTableFastSS& table;
				std::string key_buffer;
				std::string first_word_buffer;
				std::string second_word_buffer;

				unsigned first_word_index;
				unsigned first_error_mask;

				unsigned second_word_index;
				unsigned second_error_mask;

			public:
				TermDataCompare(const BrassSpellingTableFastSS& table_) :
					table(table_)
				{
				}

				bool operator()(unsigned first_term, unsigned second_term);
		};

		class TermIndexCompare
		{
				std::vector<std::vector<unsigned> >& wordlist_map;

				unsigned first_word_index;
				unsigned first_error_mask;

				unsigned second_word_index;
				unsigned second_error_mask;

			public:
				TermIndexCompare(const TermIndexCompare& other) :
					wordlist_map(other.wordlist_map)
				{
				}

				TermIndexCompare(std::vector<std::vector<unsigned> >& wordlist_map_) :
					wordlist_map(wordlist_map_)
				{
				}

				bool operator()(unsigned first_term, unsigned second_term);
		};

		static void get_word_key(unsigned index, string& key);

		static unsigned get_data_int(const string& data, unsigned index);

		static void append_data_int(string& data, unsigned value);

		unsigned term_binary_search(const string& data, const std::vector<unsigned>& word, unsigned error_mask,
				unsigned start, unsigned end, bool lower);

		void toggle_term(const std::vector<unsigned>& word, string& prefix, unsigned index, unsigned error_mask,
				bool update_prefix);

		void toggle_recursive_term(const std::vector<unsigned>& word, string& prefix, unsigned index,
				unsigned error_mask, unsigned start, unsigned distance, unsigned max_distance);

		//Search for a word and fill result set
		void populate_term(const std::vector<unsigned>& word, string& data, string& prefix, unsigned error_mask,
				bool update_prefix, std::tr1::unordered_set<unsigned>& result);

		//Recursively search for a word with 0, 1, ..., max_distance errors.
		void populate_recursive_term(const std::vector<unsigned>& word, string& data, string& prefix,
				unsigned error_mask, unsigned start, unsigned distance, unsigned max_distance, std::tr1::unordered_set<
						unsigned>& result);

		//Generate prefix of a word using given error mask.
		void get_term_prefix(const std::vector<unsigned>& word, string& prefix, unsigned error_mask,
				unsigned prefix_length);

		static unsigned pack_term_index(unsigned wordindex, unsigned error_mask);
		static void unpack_term_index(unsigned termindex, unsigned& wordindex, unsigned& error_mask);

		template<typename FirstIt, typename SecondIt>
		static int compare_string(FirstIt first_it, FirstIt first_end, SecondIt second_it, SecondIt second_end,
				unsigned first_error_mask, unsigned second_error_mask, unsigned limit)
		{
			unsigned first_i = 0;
			unsigned second_i = 0;

			for (;; ++first_it, ++second_it, ++first_i, ++second_i, first_error_mask >>= 1, second_error_mask >>= 1)
			{
				bool first_at_end = false;
				while (!(first_at_end = (first_i == limit || first_it == first_end)) && (first_error_mask & 1))
				{
					first_error_mask >>= 1;
					++first_i;
					++first_it;
				}

				bool second_at_end = false;
				while (!(second_at_end = (second_i == limit || second_it == second_end)) && (second_error_mask & 1))
				{
					second_error_mask >>= 1;
					++second_i;
					++second_it;
				}

				if (first_at_end && second_at_end)
					return 0;

				if (first_at_end && !second_at_end)
					return -1;

				if (!first_at_end && second_at_end)
					return 1;

				if (*first_it < *second_it)
					return -1;

				if (*first_it > *second_it)
					return 1;
			}
		}

		std::vector<std::vector<unsigned> > wordlist_map;
		std::map<string, std::vector<unsigned> > termlist_deltas;

	protected:
		void merge_fragment_changes();

		void toggle_word(const string& word);

		void populate_word(const string& word, unsigned max_distance, std::vector<TermList*>& result);

	public:
		BrassSpellingTableFastSS(const std::string & dbdir, bool readonly) :
			BrassSpellingTable(dbdir, readonly), wordlist_map(), termlist_deltas()
		{
		}

		bool get_word(unsigned index, string& key, string& word) const;

		/** Override methods of BrassSpellingTable.key
		 *
		 *  NB: these aren't virtual, but we always call them on the subclass in
		 *  cases where it matters.
		 *  @{
		 */
		void cancel()
		{
			wordlist_map.clear();
			termlist_deltas.clear();
			BrassSpellingTable::cancel();
		}
		// @}
};

class BrassSpellingFastSSTermList : public TermList
{
		const BrassSpellingTableFastSS& table;
		std::vector<unsigned> words;
		std::string word;
		std::string key_buffer;
		unsigned index;

	public:
		BrassSpellingFastSSTermList(const std::vector<unsigned>& words_, const BrassSpellingTableFastSS& table_) :
			table(table_), words(words_), index(0)
		{
		}

		Xapian::termcount get_approx_size() const;

		std::string get_termname() const;

		Xapian::termcount get_wdf() const;

		Xapian::doccount get_termfreq() const;

		Xapian::termcount get_collection_freq() const;

		TermList * next();

		TermList * skip_to(const std::string & term);

		bool at_end() const;

		Xapian::termcount positionlist_count() const;

		Xapian::PositionIterator positionlist_begin() const;
};

#endif // XAPIAN_INCLUDED_BRASS_SPELLING_FASTSS_H