#pragma once

#include <string>
#include <vector>

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>

namespace boost {
	namespace spirit {
		namespace traits
		{
			/* cast for Symbol -> std::string
			 * The data is just restructured.
			 */
			template <>
			struct transform_attribute<std::string, boost::fusion::vector<char, std::vector<char>>, qi::domain>
			{
				static int pre(std::string& d) { return 0; }//not useful in this case but required to avoid compiler errors
				static void post(std::string& val, boost::fusion::vector<char, std::vector<char>> const& attr) //`val` is the "returned" string, `attr` is what int_ parses
				{
					using namespace boost::fusion;
					val.resize(at_c<1>(attr).size() + 1);
					val[0] = at_c<0>(attr);
					for (int i = 0; i < at_c<1>(attr).size(); ++i)
						val[i + 1] = at_c<1>(attr)[i];

				}
				static void fail(std::string&) {}
			};

			//RExpression -> string (just the operator)
			template <>
			struct transform_attribute<std::string, boost::fusion::vector<std::string, boost::optional<std::string>>, qi::domain>
			{
				static int pre(std::string& d) { return 0; }//not useful in this case but required to avoid compiler errors
				static void post(std::string& val, boost::fusion::vector<std::string, boost::optional<std::string>> const& attr) //`val` is the "returned" string, `attr` is what  parses
				{
					val = boost::fusion::at_c<0>(attr); //discard the optional string
				}
				static void fail(std::string&) {}
			};


/*			//TypeInfo
			template <>
			struct transform_attribute<par::TypeInfo, boost::fusion::vector3<boost::optional<std::string>, std::string, boost::optional<char>>, qi::domain>
			{
				static int pre(std::string& d) { return 0; }//not useful in this case but required to avoid compiler errors
				static void post(par::TypeInfo& val, boost::fusion::vector3<boost::optional<std::string>, std::string, boost::optional<char>> const& attr) //`val` is the "returned" string, `attr` is what  parses
				{
					val
				}
				static void fail(std::string&) {}
			};*/
		}
	}
}