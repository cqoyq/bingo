/*
 * parse_handler.h
 *
 *  Created on: 2016-2-19
 *      Author: root
 */

#ifndef CFG_JSON_PARSE_HANDLER_H_
#define CFG_JSON_PARSE_HANDLER_H_

#include <iostream>

#include "../../foreach_.h"
#include "../node.h"
using bingo::cfg::node;

#include <boost/property_tree/json_parser.hpp>
namespace pt = boost::property_tree;
using namespace boost;

#include <boost/exception/all.hpp>
#include <boost/current_function.hpp>

namespace bingo { namespace cfg { namespace json {

struct json_parser{
	static bool is_pretty;
};

// Json parse without attribute
template<typename PARSER = json_parser>
class json_parse_handler {
public:
	json_parse_handler(){
		node_ = 0;
	}
	virtual ~json_parse_handler(){
		if(node_ != 0) delete node_;
	}

	int read(const char* file, string& err){
		try{
			pt::read_json(file, pt_);
			return 0;
		}
		catch(boost::exception& e){
			err = current_exception_cast<std::exception>()->what();
			return -1;
		}
	}

	int read(stringstream& stream, string& err){
		try{
			pt::read_json(stream, pt_);
			return 0;
		}
		catch(boost::exception& e){
			err = current_exception_cast<std::exception>()->what();
			return -1;
		}
	}


	const char* get_value(const char* path, string& err){

		try{
			str_ = pt_.get<string>(path);
			return str_.c_str();
		}
		catch(boost::exception& e){
			err = current_exception_cast<std::exception>()->what();
			return 0;
		}
	}

	node* get_node(const char* path, string& err){
		if(node_ !=0) {
			delete node_;
			node_ = 0;
		}

		try{
			pt::ptree pt = pt_.get_child(path);

			node_ = new node();
			parse_ptree(node_, pt);

			return node_;
		}
		catch(boost::exception& e){
			err = current_exception_cast<std::exception>()->what();
			return 0;
		}
	}



	void write(stringstream& stream, node* n){

		pt::ptree wtree;
		make_ptree(n, wtree);

		pt::write_json(stream, wtree, PARSER::is_pretty);
	}

	void write(const char* file, node* n){

		pt::ptree wtree;
		make_ptree(n, wtree);

		pt::write_json(file, wtree, std::locale(), PARSER::is_pretty);
	}
private:
	void parse_ptree(node* n, pt::ptree& tree){
		n->value.append(tree.get_value<string>().c_str());

		foreach_(pt::ptree::value_type &v1, tree){
			 if(v1.first == "<xmlattr>"){

			 }else{
				 // First node do it.
				 string name(v1.first);
				 node* child = new node(name.c_str());

				 parse_ptree(child, v1.second);

				 n->child.push_back(child);
			 }

		}
	}

	void make_ptree(node* n, pt::ptree& tree){

		pt::ptree& addnode = tree.add(n->name, n->value);

		if(n->child.set.size() > 0){
			// Have child, then node value is ''.
			addnode.put_value("");

			foreach_(node& m, n->child.set){
				make_ptree(&m, addnode);
			}
		}
	}
private:
	pt::ptree pt_;
	node* node_;
	string str_;
};

} } }


#endif /* CFG_JSON_PARSE_HANDLER_H_ */
