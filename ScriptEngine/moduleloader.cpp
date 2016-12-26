#include <fstream>
#include "moduleloader.hpp"
#include "logger.hpp"

namespace NaReTi{
	using namespace std;

	std::string ModuleLoader::load(const std::string& _name) const
	{
		string fileName = m_config.scriptLocation + _name + ".nrt";
		std::ifstream in(fileName.c_str());

		//check if file is valid
		if (!in)
		{
			LOG(Error, "Could not open module-file " << fileName);
			return "#";
		}

		string fileContent;

		// put filecontent into a string
		in.seekg(0, std::ios::end);
		fileContent.reserve(in.tellg());
		in.seekg(0, std::ios::beg);

		fileContent.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

		return std::move(fileContent);
	}
}