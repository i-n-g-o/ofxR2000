//
//  optional.h
//  R2000Example
//
//  Created by inx on 01.06.19.
//

#ifndef optional_h
#define optional_h

namespace pepperl_fuchs {
	
	template< class T >
	class optional {
	public:
		optional() : m_hasValue(false) {}
		optional(const T& value) : m_hasValue(true), m_value(value) {}
		
		bool hasValue() const { return m_hasValue; }
		bool isSpecified() { return m_hasValue; }
		T value() { return m_value; }
		
	private:
		bool m_hasValue;
		T m_value;
		
	};
}

#endif /* optional_h */
