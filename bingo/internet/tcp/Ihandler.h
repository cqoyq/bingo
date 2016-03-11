/*
 * Ihandler.h
 *
 *  Created on: 2016-3-2
 *      Author: root
 */

#ifndef TCP_IHANDLER_H_
#define TCP_IHANDLER_H_

namespace bingo { namespace internet { namespace tcp {

class IServerhandler{
public:
	virtual ~IServerhandler(){};

	virtual void check_heartjump() = 0;
	virtual void check_authentication_pass() = 0;
};

} } }


#endif /* TCP_IHANDLER_H_ */
