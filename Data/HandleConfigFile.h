/*
 * HandleConfigFile.h
 *
 *  Created on: Oct 29, 2013
 *      Author: user
 */

#ifndef HandleConfigFile_H_
#define HandleConfigFile_H_

#include <vector>
#include <map>
#include <string.h>
#include <iostream>
#include <boost/thread.hpp>

class HandleConfigFile
{
  public:
    static HandleConfigFile* instance_;

    static void createInstance();

    virtual ~HandleConfigFile();

    std::string getParameter(std::string option);

    void setOption(std::string option, std::string value);

  private:
    HandleConfigFile(const HandleConfigFile& handler);
    std::vector<std::string> available_options_;
    std::vector<std::string> lines_;
    boost::mutex parameter_map_lock_;
    std::map<std::string, std::string> parameter_map_;
    std::string filename_;
    static boost::mutex instance_lock_;

    HandleConfigFile(std::string filename);

    /**
     * @brief readConfig reads the config file which was specified by filename_
     */
    void readConfig();

    /**
     * @brief writeConfig writes the config back to file with name specified by filename_
     */
    void writeConfig();

    /**
     * parses the given line and returns the option
     */
    std::string parseOption(std::string line);

    bool isOption(std::string line);

    /**
     * parse line for type
     * @return true if line is of given type, false otherwise
     */
    bool isType(std::string line, std::string type);
};

#endif /* HandleConfigFile_H_ */
