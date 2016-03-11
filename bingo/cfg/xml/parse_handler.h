/*
 * parse_handler.h
 *
 *  Created on: 2016-2-19
 *      Author: root
 */

#ifndef CFG_XML_PARSE_HANDLER_H_
#define CFG_XML_PARSE_HANDLER_H_

#include "../../foreach_.h"
#include "../node.h"
using bingo::cfg::node;

#include <boost/exception/all.hpp>
#include <boost/current_function.hpp>

#include <boost/property_tree/xml_parser.hpp>
namespace pt = boost::property_tree;
using namespace boost;

namespace bingo { namespace cfg { namespace xml {

struct xml_parser{
	static bool is_pretty;
};

template<typename PARSER = xml_parser>
class xml_parse_handler {
public:
	xml_parse_handler(){
		node_ = 0;
	}
	virtual ~xml_parse_handler(){
		if(node_ != 0) delete node_;
	}

	int read(const char* file, string& err){
		try{
			pt::read_xml(file, pt_, pt::xml_parser::no_comments);
			return 0;
		}
		catch(boost::exception& e){
			err = current_exception_cast<std::exception>()->what();
			return -1;
		}
	}

	int read(stringstream& stream, string& err){
		try{
			pt::read_xml(stream, pt_, pt::xml_parser::no_comments);
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

	const char* get_attr(const char* path, const char* attrname, string& err){
		try
		{
			bool is_find = false;

			pt::ptree pt = pt_.get_child(path);
			foreach_(pt::ptree::value_type &v1, pt){
				 if(v1.first == "<xmlattr>"){
					 foreach_(pt::ptree::value_type &vAttr, v1.second){
						 if(vAttr.first.compare(attrname) == 0){
							 str_ = vAttr.second.data();
							 is_find = true;
							 break;
						 }
					 }
					 if(is_find) break;
				 }
			}
			if(!is_find) {
				BOOST_THROW_EXCEPTION(attr_not_exist_exp());
			}
			return str_.c_str();
		}
		catch(boost::exception& e)
		{
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

		if(PARSER::is_pretty){
			boost::property_tree::xml_writer_settings<std::string> settings('\t', 1);
			pt::write_xml(stream, wtree, settings);
		}else
			pt::write_xml(stream, wtree);
	}

	void write(const char* file, node* n){

		pt::ptree wtree;
		make_ptree(n, wtree);

		if(PARSER::is_pretty){
			boost::property_tree::xml_writer_settings<std::string> settings('\t', 1);
			write_xml(file, wtree, std::locale(), settings);
		}else
			write_xml(file, wtree, std::locale());
	}
private:
	void parse_ptree(node* n, pt::ptree& tree){
		n->value.append(tree.get_value<string>().c_str());

		foreach_(pt::ptree::value_type &v1, tree){
			 if(v1.first == "<xmlattr>"){
				 foreach_(pt::ptree::value_type &vAttr, v1.second){
					 // first do it.
					 string name(vAttr.first);
					 string value(vAttr.second.data());

					 n->attrs.push_back(new node_attr(name.c_str(),value.c_str()));
				 }
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


		if(n->attrs.set.size() > 0){

			pt::ptree& addattr = addnode.add("<xmlattr>","");
			foreach_(node_attr& k, n->attrs.set){
				// Create attribute node.
				pt::ptree::value_type tvalue(pair<string, pt::ptree>(k.name,pt::ptree()));
				tvalue.second.put_value(k.value);
				addattr.push_back(tvalue);
			}
		}

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


#endif /* CFG_XML_PARSE_HANDLER_H_ */
