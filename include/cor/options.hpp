#ifndef _COR_OPTIONS_HPP_
#define _COR_OPTIONS_HPP_
/*
 * Command line options parser
 *
 * Copyright (C) 2012 Jolla Ltd.
 * Contact: Denis Zalevskiy <denis.zalevskiy@jollamobile.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <map>
#include <functional>
#include <algorithm>
#include <vector>
#include <set>
#include <stdexcept>
#include <cstring>

namespace cor
{

template <class StringT>
class OptParse
{
public:
    typedef std::map<char, char const*> short_opts_type;
    typedef typename short_opts_type::value_type short_item_type;
    typedef std::map<StringT, char const*> map_type;
    typedef typename map_type::value_type item_type;

    /**
     * @short_opts list of pairs (short_item_char, item_name)
     * @long_opts list of pairs (long_item_string, item_name)
     * @opt_with_params set of item names has params
     * @leave_in_params set of item names should be also left in params
     */
    OptParse(std::initializer_list<short_item_type> short_opts,
             std::initializer_list<item_type> long_opts,
             std::initializer_list<StringT> opt_with_params,
             std::initializer_list<StringT> leave_in_params = {})
        : short_opts_(short_opts),
          long_opts_(long_opts),
          opt_with_params_(opt_with_params),
          leave_in_params_(leave_in_params)
    { }

    void show_help(std::ostream &out, char const *program_name,
                   StringT const &usage_str = "")
    {
        if (usage_str.size()) {
            out << "Usage: " << program_name << usage_str;
        } else {
            out << "Usage: " << program_name << " [options]\n"
                << "\twhere [options] are:\n";
        }
        std::map<StringT, std::pair<char, StringT> > grouped;
        for (auto p : short_opts_)
            grouped[p.second].first = p.first;
        for (auto p : long_opts_)
            grouped[p.second].second = p.first;
        for (auto p : grouped) {
            auto &sl = p.second;
            out << "\t";
            if (sl.first) {
                out << '-' << sl.first;
                if (opt_with_params_.count(p.first))
                    out << " <" << p.first << '>';
            }
            out << "\t";
            if (sl.second.size()) {
                out << "--" << sl.second;
                if (opt_with_params_.count(p.first))
                    out << " <" << p.first << '>';
            }
            out << "\n";
        }
    }

    /** main method to perform options parsing
     *
     * @opts map to gather (item_name, value) association
     * @params container to gather parameters
     */
    int parse(int argc, char *argv[],
              map_type &opts,
              std::vector<char const*> &params)
    {
        enum stages {
            option,
            opt_param
        };
        stages stage = option;
        StringT name;
        bool is_leave_param = false;

        auto parse_short = [&](char const *s, size_t len) {
            auto p = short_opts_.find(s[1]);
            if (p == short_opts_.end()) {
                params.push_back(s);
                return;
            }

            name = p->second;

            is_leave_param = (leave_in_params_.count(name) != 0);
            if (is_leave_param)
                params.push_back(s);

            if (!opt_with_params_.count(name)) {
                opts[name] = nullptr;
            } else if (len > 2) {
                opts[name] = &s[2];
                if (is_leave_param)
                    params.push_back(&s[2]);
            } else {
                stage = opt_param;
            }
        };

        auto parse_long = [&](char const *s, size_t) {
            auto p = long_opts_.find(&s[2]);
            if (p == long_opts_.end()) {
                params.push_back(s);
                return;
            }
            name = p->second;

            is_leave_param = (leave_in_params_.count(name) != 0);
            if (is_leave_param)
                params.push_back(s);

            auto p_has = opt_with_params_.find(name);
            if (p_has == opt_with_params_.end())
                opts[name] = 0;
            else
                stage = opt_param;
        };

        auto parse_opt_param = [&](char const *s, size_t len) {
            if (len >= 1 && s[0] == '-')
                throw std::logic_error(s);
            opts[name] = s;
            if (is_leave_param)
                params.push_back(s);
            stage = option;
        };

        auto parse_option = [&](char const *s, size_t len) {
            if (s[0] == '-') {
                if (len > 3 && s[1] == '-')
                    parse_long(s, len);
                else
                    parse_short(s, len);
            } else {
                params.push_back(s);
            }
        };

        auto process = [&](char *v) {
            size_t len = std::strlen(v);
            if (!len)
                return;
            switch (stage) {
            case option:
                return parse_option(v, len);
            case opt_param:
                return parse_opt_param(v, len);
            };
        };

        std::for_each(argv, argv + argc, process);
        return 0;
    }

private:
    short_opts_type short_opts_;
    map_type long_opts_;
    std::set<StringT> opt_with_params_;
    std::set<StringT> leave_in_params_;
};

} // cor

#endif // _COR_OPTIONS_HPP_
