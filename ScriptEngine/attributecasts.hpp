#pragma once

#include <string>
#include <vector>

#include <boost/spirit/include/qi.hpp>

namespace boost {
	namespace spirit {
		namespace traits
		{
			/* cast for Symbol -> std::string
			 * The data is just restructured.
			 */
			template <>
			struct transform_attribute<std::string, boost::fusion::vector2<char, std::vector<char>>, qi::domain>
			{
				static int pre(std::string& d) { return 0; }//not useful in this case but required to avoid compiler errors
				static void post(std::string& val, boost::fusion::vector2<char, std::vector<char>> const& attr) //`val` is the "returned" string, `attr` is what int_ parses
				{
					val.resize(attr.m1.size() + 1);
					val[0] = attr.m0;
					for (int i = 0; i < attr.m1.size(); ++i)
						val[i + 1] = attr.m1[i];

				}
				static void fail(std::string&) {}
			};

			//RExpression -> string (just the operator)
			template <>
			struct transform_attribute<std::string, boost::fusion::vector2<std::string, boost::optional<std::string>>, qi::domain>
			{
				static int pre(std::string& d) { return 0; }//not useful in this case but required to avoid compiler errors
				static void post(std::string& val, boost::fusion::vector2<std::string, boost::optional<std::string>> const& attr) //`val` is the "returned" string, `attr` is what  parses
				{
					val = attr.m0; //discard the optional string
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