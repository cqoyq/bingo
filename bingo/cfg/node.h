/*
 * node.h
 *
 *  Created on: 2016-2-19
 *      Author: root
 */

#ifndef CFG_NODE_H_
#define CFG_NODE_H_

#include <exception>
#include <string>
using namespace std;

#include <boost/ptr_container/ptr_vector.hpp>

namespace bingo { namespace cfg {


struct attr_not_exist_exp : public std::exception {
	attr_not_exist_exp() : std::exception(){};

	const char* what()const throw()
   {
		return "attribute isn't exit!";
   }
};

struct node_attr{
	string name;
	string value;
	node_attr(const char* name, const char* value):
		name(name), value(value){}
};

struct node_attr_set{
	node_attr_set& push_back(node_attr* n){
		set.push_back(n);
		return (*this);
	}

	boost::ptr_vector<node_attr> set;
};

class node;
struct node_set{

	node* push_back(node* n){
		set.push_back(n);
		return n;
	}

	boost::ptr_vector<node> set;
};


struct node{
	string 							name;
	string 							value;
	node_attr_set				 	attrs;
	node_set				 		child;

	node(){}
	node(const char* name, const char* value = ""):
		name(name), value(value){};

	template<typename HANDLER>
	void write(stringstream& stream){
		HANDLER hdr;
		hdr.write(stream, this);
	}

	template<typename HANDLER>
	void write(const char* file){
		HANDLER hdr;
		hdr.write(file, this);
	}

private:

};

}  }


#endif /* CFG_NODE_H_ */
