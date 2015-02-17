/*
 * HandleConfigFile.cpp
 *
 *  Created on: Oct 29, 2013
 *      Author: user
 */

#include <stdio.h>

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Data/HandleConfigFile.h"

HandleConfigFile* HandleConfigFile::instance_ = 0;
boost::mutex HandleConfigFile::instance_lock_;

HandleConfigFile::HandleConfigFile(std::string filename) :
    filename_(filename)
{
  available_options_.push_back("ip");
  available_options_.push_back("port");
  available_options_.push_back("size_of_ringbuffer");
  available_options_.push_back("update_intervall");
  readConfig();
}

HandleConfigFile::~HandleConfigFile()
{
  writeConfig();
}

void HandleConfigFile::createInstance()
{
  boost::unique_lock<boost::mutex> lck(instance_lock_);
  if (!instance_) {
    instance_ = new HandleConfigFile("client.cfg");
  }
}

bool HandleConfigFile::isType(std::string line, std::string type)
{
  // line could look like "email=" or "email =" or " email =" or "   email="
  size_t pos = line.find_first_not_of(" ");
  if(pos != std::string::npos && (!line.compare(pos, line.find(" ", 0), type) ||
     !line.compare(pos, line.find("=", 0), type)))
    return true;
  return false;
}

bool HandleConfigFile::isOption(std::string line)
{
  for (std::vector<std::string>::iterator it = available_options_.begin();
       it != available_options_.end();
       it++) {
    if (isType(line, *it)) {
      std::string param = parseOption(line);
      if (param.size()) {
        std::cout << "[HandleConfigFile::isOption] read option: " << *it << ", with parameter " << param << std::endl;
        parameter_map_[*it] = param;
        return true;
      }
    }
  }
  return false;
}

void HandleConfigFile::readConfig()
{
  std::string line;
  int counter = 0;
  std::ifstream infile(filename_.c_str());
  if (infile.is_open()) {
    while (std::getline(infile, line))
    {
      ++counter;
      std::cout << "line is: " << line << std::endl;
      lines_.push_back(line);
      // see what kind of line: if it starts with # its comment,
      //                        email = xxxxx email addresses, can be comma separated
      //                        subject = xxxx subject
      //                        from = xxxx from address
      if (!(line.compare(0,1,"#")))
      {
        // comment, do nothing...
        continue;
      }
      else if (line.find_first_not_of(" ",0) == line.find("\n",0))
      {
        // only new line, do nothing!! can contain blanks... nothing else!
        continue;
      }
      else if(isOption(line))
      {
        // was option, everything fine
        continue;
      }
      else
      {
        // not a valid config parameter!
        std::cerr
            << "invalid configuration file - check your configuration! Error occured in line" << counter << ", Filename was: "
            << filename_ << std::endl;
        break;
      }
    }
    infile.close();
  }
}

std::string HandleConfigFile::parseOption(std::string line)
{
  std::string param;
  size_t pos2 = 0;
  size_t pos = line.find("=", 0);
  pos = line.find_first_not_of(" ", pos + 1);
  if (pos != std::string::npos) {
    pos2 = line.find("\n", pos);
    if(pos2 == std::string::npos)
    {
      pos2 = line.size();
    }
    //std::cout << "pos is: " << pos << std::endl;
    //std::cout << "pos2 is: " << pos2 << std::endl;
    param = line.substr(pos, pos2 - pos);
  }
  return param;
}

void HandleConfigFile::writeConfig()
{
  // write configuration back to file
}


std::string HandleConfigFile::getParameter(std::string option)
{
  boost::unique_lock<boost::mutex> lck(parameter_map_lock_);
  std::string param;
  try {
    param = parameter_map_.at(option);
  } catch (std::out_of_range e) {
    std::cout << "no value found for option " << option << std::endl;
  }

  return param;
}

void HandleConfigFile::setOption(std::string option, std::string value)
{
  boost::unique_lock<boost::mutex> lck(parameter_map_lock_);
  parameter_map_[option] = value;
}
