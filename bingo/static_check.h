/*
 * static_check.h
 *
 *  Created on: 2016-2-1
 *      Author: root
 */

#ifndef STATIC_CHECK_H_
#define STATIC_CHECK_H_

namespace bingo {

	#define static_check_size_t(t) 	(sizeof(t) == sizeof(size_t))?true:false
	#define static_check_u8(t) 		(sizeof(t) == sizeof(bingo::u8))?true:false
	#define static_check_u16(t) 	(sizeof(t) == sizeof(bingo::u16))?true:false
	#define static_check_u32(t) 	(sizeof(t) == sizeof(bingo::u32))?true:false
	#define static_check_u64(t) 	(sizeof(t) == sizeof(bingo::u64))?true:false


	template<typename T, typename U>
	struct class_inherit
	{
		typedef char Small;
		class Big{
			char dummy[2];
		};

		static Small Test(U);
		static Big Test(...);
		static T makeT();
	public:
		enum{exists=sizeof(Test(makeT()))==sizeof(Small)};
	};
	#define static_check_class_inherit(t, base) class_inherit<t, base>::exists
	#define static_check_greater_than_zero(a) (a>0)?true:false

}


#endif /* STATIC_CHECK_H_ */
