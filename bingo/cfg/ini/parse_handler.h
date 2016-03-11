/*
 * parse_handler.h
 *
 *  Created on: 2016-2-20
 *      Author: root
 */

#ifndef CFG_INI_PARSE_HANDLER_H_
#define CFG_INI_PARSE_HANDLER_H_

#include <boost/property_tree/ini_parser.hpp>
namespace pt = boost::property_tree;
using namespace boost;

namespace bingo { namespace cfg { namespace ini {

class ini_parse_handler {
public:
	void write(stringstream& stream, node* n){

		pt::ptree wtree;

		make_ptree(n, wtree, 1);

		pt::write_ini(stream, wtree);
	}

	void write(const char* file, node* n){

		pt::ptree wtree;
		make_ptree(n, wtree, 1);

		pt::write_ini(file, wtree);
	}
private:
	void make_ptree(node* n, pt::ptree& tree, int level){

		if(level == 3) return;

		if(n->child.set.size() > 0){

			foreach_(node& m, n->child.set){
				pt::ptree& addnode = tree.add(m.name, m.value);
				int nlevel = level + 1;
				make_ptree(&m, addnode, nlevel);
			}
		}
	}
};

} } }

#endif /* CFG_INI_PARSE_HANDLER_H_ */
