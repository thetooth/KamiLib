/*
Copyright (c) 2011 Michael Nicolella
Single file conversion by Jeffrey Jenner

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <istream>
#include <iostream>
#include <ostream>

#define jsonInternalAssert(Expression) json::detail::InternalAssert( Expression, #Expression )

namespace json
{
	typedef std::int64_t int_t;
	typedef float        float_t;
	struct Value;
	struct Member;

	struct JsonException : std::runtime_error
	{
		explicit JsonException(std::string const& message)
			:std::runtime_error(message)
		{
		}

		explicit JsonException(char const* message)
			:std::runtime_error(message)
		{
		}
	};


	namespace detail
	{

		template<typename T>
		void unused_arg( T const& t )
		{
			(void)t;
		}

		inline void RaiseException( std::string const& message )
		{
			throw json::JsonException(message);
		}


		inline void InternalAssert( bool condition, char const* expressionStr )
		{
			if( !condition )
			{
				RaiseException(expressionStr);
			}
		}


		struct ValueString
		{
			std::string data;

			ValueString() {}
			ValueString( ValueString const& rhs ):data(rhs.data){}
			ValueString( ValueString&& rhs      ):data(std::move(rhs.data)){}

			ValueString& operator=( ValueString const& rhs ){data = rhs.data;return *this;}
			ValueString& operator=( ValueString&& rhs      ){data = std::move(rhs.data);return *this;}

			template<typename StringT>
			ValueString( StringT&& data, typename std::enable_if< !std::is_same<StringT,ValueString&>::value >::type* = 0 )
				:data(std::forward<StringT>(data))
			{
			}
		};

		struct ValueInt
		{
			json::int_t data;
		};

		struct ValueFloat
		{
			json::float_t data;
		};

		struct ValueBool
		{
			bool data;
		};

		struct ValueArray
		{
			std::vector<Value> elements;

			ValueArray(){}
			ValueArray( ValueArray const& rhs ):elements(rhs.elements){}
			ValueArray( ValueArray&& rhs      ):elements(std::move(rhs.elements)){}

			ValueArray& operator=( ValueArray const& rhs )
			{
				elements = rhs.elements;
				return *this;
			}

			ValueArray& operator=( ValueArray&& rhs )
			{
				elements = std::move(rhs.elements);
				return *this;
			}
		};

		struct ValueObject
		{
			std::vector<json::Member> members;

			ValueObject(){}
			ValueObject( ValueObject const& rhs ):members(rhs.members){}
			ValueObject( ValueObject&& rhs      ):members(std::move(rhs.members)){}

			ValueObject& operator=( ValueObject const& rhs )
			{
				members = rhs.members;
				return *this;
			}

			ValueObject& operator=( ValueObject&& rhs )
			{
				members = std::move(rhs.members);
				return *this;
			}
		};


		template<std::size_t Lhs, std::size_t Rhs>
		struct max_
		{
			static std::size_t const value = Lhs > Rhs ? Lhs : Rhs;
		};


		struct find_largest_value_size
		{
			static std::size_t const value = max_<sizeof(ValueString)
				,max_<sizeof(ValueInt)
				,max_<sizeof(ValueFloat)
				,max_<sizeof(ValueBool)
				,max_<sizeof(ValueArray),sizeof(ValueObject)
				>::value
				>::value
				>::value
				>::value
			>::value;
		};


	}

	enum ValueType
	{
		kValueNull
		,kValueString
		,kValueInt
		,kValueFloat
		,kValueBool
		,kValueArray
		,kValueObject
	};


	struct Value
	{
		char data[detail::find_largest_value_size::value];
		ValueType type;

		Value()
			:type(kValueNull)
		{
		}

		Value( Value const& rhs )
			:type(kValueNull)
		{
			*this = rhs;
		}

		Value( Value&& rhs )
			:type(kValueNull)
		{
			*this = std::move(rhs);
		}

		Value& operator=( Value const& rhs )
		{
			switch(rhs.type)
			{
			case kValueString: *InternalMakeString() = *rhs.InternalGetString(); break;
			case kValueInt:    *InternalMakeInt()    = *rhs.InternalGetInt();    break;
			case kValueFloat:  *InternalMakeFloat()  = *rhs.InternalGetFloat();  break;
			case kValueObject: *InternalMakeObject() = *rhs.InternalGetObject(); break;
			case kValueArray:  *InternalMakeArray()  = *rhs.InternalGetArray();  break;
			case kValueBool:   *InternalMakeBool()   = *rhs.InternalGetBool();   break;
			case kValueNull:   InternalMakeNull();                               break;
			}
			return *this;
		}

		Value& operator=( Value&& rhs )
		{
			DestructValue(type,data);

			type = rhs.type;

			switch(type)
			{
			case kValueString: InternalMoveString(std::move(*rhs.InternalGetString())); break;
			case kValueInt:    InternalMoveInt   (std::move(*rhs.InternalGetInt()));    break;
			case kValueFloat:  InternalMoveFloat (std::move(*rhs.InternalGetFloat()));  break;
			case kValueObject: InternalMoveObject(std::move(*rhs.InternalGetObject())); break;
			case kValueArray:  InternalMoveArray (std::move(*rhs.InternalGetArray()));  break;
			case kValueBool:   InternalMoveBool  (std::move(*rhs.InternalGetBool()));   break;
			case kValueNull:   InternalMakeNull();                                      break;
			}

			return *this;
		}

		~Value()
		{
			DestructValue(type,data);
		}



		inline bool IsNull()   const { return type == kValueNull;   }
		inline bool IsString() const { return type == kValueString; }
		inline bool IsInt()    const { return type == kValueInt;    }
		inline bool IsFloat()  const { return type == kValueFloat;  }
		inline bool IsBool()   const { return type == kValueBool;   }
		inline bool IsArray()  const { return type == kValueArray;  }
		inline bool IsObject() const { return type == kValueObject; }


		//
		// standard value
		//

		void SetNull  ()                           { InternalMakeNull();                 }
		void SetInt   ( json::int_t value )        { InternalMakeInt()->data = value;    }
		void SetFloat ( json::float_t value )      { InternalMakeFloat()->data = value;  }
		void SetBool  ( bool value )               { InternalMakeBool()->data = value;   }
		void SetArray ()                           { InternalMakeArray();                }
		void SetObject()                           { InternalMakeObject();               }
		template<typename StringT>
		void SetString( StringT&& value )          { InternalMakeString(std::forward<StringT>(value)); }

		std::string const& GetString() const { return InternalGetString()->data; }
		json::int_t        GetInt()    const { return InternalGetInt()->data;    }
		json::float_t      GetFloat()  const { return InternalGetFloat()->data;  }
		bool               GetBool()   const { return InternalGetBool()->data;   }


		//
		// Object accessors
		//

		template<typename StringT> void SetValueMember ( StringT&& member, Value const& value )       { GetOrAddMember(std::forward<StringT>(member), value); }
		template<typename StringT> void SetValueMember ( StringT&& member, Value&&      value )       { GetOrAddMember(std::forward<StringT>(member), std::move(value)); }
		template<typename StringT> void SetNullMember  ( StringT&& member )                           { GetOrAddMember(std::forward<StringT>(member)).SetNull(); }
		template<typename StringT, typename StringValueT> void SetStringMember( StringT&& member, StringValueT&& value ) { GetOrAddMember(std::forward<StringT>(member)).SetString(std::forward<StringValueT>(value)); }
		template<typename StringT> void SetIntMember   ( StringT&& member, json::int_t value )        { GetOrAddMember(std::forward<StringT>(member)).SetInt(value); }
		template<typename StringT> void SetFloatMember ( StringT&& member, json::float_t value )      { GetOrAddMember(std::forward<StringT>(member)).SetFloat(value); }
		template<typename StringT> void SetBoolMember  ( StringT&& member, bool value )               { GetOrAddMember(std::forward<StringT>(member)).SetBool(value); }
		template<typename StringT> void SetArrayMember ( StringT&& member, Value const& value )       { GetOrAddMember(std::forward<StringT>(member)) = value; }
		template<typename StringT> void SetObjectMember( StringT&& member, Value const& value )       { GetOrAddMember(std::forward<StringT>(member)) = value; }

		template<typename StringT> std::string const& GetStringMember ( StringT&& memberName ) const { return GetMemberValue(std::forward<StringT>(memberName))->GetString(); }
		template<typename StringT> json::int_t        GetIntMember    ( StringT&& memberName ) const { return GetMemberValue(std::forward<StringT>(memberName))->GetInt(); }
		template<typename StringT> json::float_t      GetFloatMember  ( StringT&& memberName ) const { return GetMemberValue(std::forward<StringT>(memberName))->GetFloat(); }
		template<typename StringT> bool               GetBoolMember   ( StringT&& memberName ) const { return GetMemberValue(std::forward<StringT>(memberName))->GetBool(); }

		template<typename StringT> Value&             GetArrayMember  ( StringT&& memberName )       { return *GetMemberValue(std::forward<StringT>(memberName)); }
		template<typename StringT> Value const&       GetArrayMember  ( StringT&& memberName ) const { return *GetMemberValue(std::forward<StringT>(memberName)); }

		template<typename StringT> Value&             GetObjectMember ( StringT&& memberName )       { return *GetMemberValue(std::forward<StringT>(memberName)); }
		template<typename StringT> Value const&       GetObjectMember ( StringT&& memberName ) const { return *GetMemberValue(std::forward<StringT>(memberName)); }

		std::size_t GetNumMembers() const { return InternalGetObject()->members.size(); }

		Member const& GetMember( std::size_t index ) const { return InternalGetObject()->members[index]; }

		template<typename StringT> bool HasMember( StringT&& memberName ) const;


		//
		// Array accessors
		//

		void AddNull  ()                           { Value v; v.SetNull();        InternalGetArray()->elements.push_back(std::move(v)); }

		template<typename StringT>
		void AddString( StringT&& value ) { Value v; v.SetString(std::forward<StringT>(value)); InternalGetArray()->elements.push_back(std::move(v)); }

		void AddInt   ( json::int_t value )        { Value v; v.SetInt(value);    InternalGetArray()->elements.push_back(std::move(v)); }
		void AddFloat ( json::float_t value )      { Value v; v.SetFloat(value);  InternalGetArray()->elements.push_back(std::move(v)); }
		void AddBool  ( bool value )               { Value v; v.SetBool(value);   InternalGetArray()->elements.push_back(std::move(v)); }
		void AddArray ( Value const& value )       { InternalGetArray()->elements.push_back(value); }
		void AddObject( Value const& value )       { InternalGetArray()->elements.push_back(value); }
		void AddValue ( Value const& value )       { InternalGetArray()->elements.push_back(value); }

		std::size_t  GetNumElements() const { return InternalGetArray()->elements.size(); }

		Value&       GetElement( std::size_t index )       { return InternalGetArray()->elements[index]; }
		Value const& GetElement( std::size_t index ) const { return InternalGetArray()->elements[index]; }

		void DelElement( std::size_t index )
		{
			detail::ValueArray* array = InternalGetArray();
			std::vector<Value>::iterator where = array->elements.begin();
			std::advance(where, index);
			array->elements.erase(where);
		}

		//is every value in this array of the same type?
		std::pair<bool, ValueType> IsHomogenousArray() const
		{
			std::pair<bool, ValueType> result;

			detail::ValueArray const* array = InternalGetArray();

			if( array->elements.empty() )
			{
				return std::make_pair(true, kValueNull);
			}

			ValueType type = array->elements[0].type;

			for( std::vector<Value>::const_iterator iter = array->elements.begin(); iter != array->elements.end(); ++iter )
			{
				if( iter->type != type )
				{
					result.first = false;
					return result;
				}
			}

			result.first  = true;
			result.second = type;

			return result;
		}



	private:
		inline void*                InternalMakeNull()   { DestructValue(type,data); type = kValueNull;   return 0; }
		template<typename StringT>
		inline detail::ValueString* InternalMakeString(StringT&& value) { DestructValue(type,data); type = kValueString; return new (data) detail::ValueString(std::forward<StringT>(value)); }
		inline detail::ValueString* InternalMakeString() { DestructValue(type,data); type = kValueString; return new (data) detail::ValueString; }
		inline detail::ValueInt*    InternalMakeInt()    { DestructValue(type,data); type = kValueInt;    return new (data) detail::ValueInt;    }
		inline detail::ValueFloat*  InternalMakeFloat()  { DestructValue(type,data); type = kValueFloat;  return new (data) detail::ValueFloat;  }
		inline detail::ValueBool*   InternalMakeBool()   { DestructValue(type,data); type = kValueBool;   return new (data) detail::ValueBool;   }
		inline detail::ValueArray*  InternalMakeArray()  { DestructValue(type,data); type = kValueArray;  return new (data) detail::ValueArray;  }
		inline detail::ValueObject* InternalMakeObject() { DestructValue(type,data); type = kValueObject; return new (data) detail::ValueObject; }

		inline void*                InternalMoveNull()                            { type = kValueNull;   return 0; }
		inline detail::ValueString* InternalMoveString(detail::ValueString&& rhs) { type = kValueString; return new (data) detail::ValueString(std::move(rhs)); }
		inline detail::ValueInt*    InternalMoveInt   (detail::ValueInt&& rhs)    { type = kValueInt;    return new (data) detail::ValueInt   (std::move(rhs)); }
		inline detail::ValueFloat*  InternalMoveFloat (detail::ValueFloat&& rhs)  { type = kValueFloat;  return new (data) detail::ValueFloat (std::move(rhs)); }
		inline detail::ValueBool*   InternalMoveBool  (detail::ValueBool&& rhs)   { type = kValueBool;   return new (data) detail::ValueBool  (std::move(rhs)); }
		inline detail::ValueArray*  InternalMoveArray (detail::ValueArray&& rhs)  { type = kValueArray;  return new (data) detail::ValueArray (std::move(rhs)); }
		inline detail::ValueObject* InternalMoveObject(detail::ValueObject&& rhs) { type = kValueObject; return new (data) detail::ValueObject(std::move(rhs)); }

		inline detail::ValueString*       InternalGetString()       { jsonInternalAssert(type == kValueString); return reinterpret_cast<detail::ValueString*>      (data); }
		inline detail::ValueString const* InternalGetString() const { jsonInternalAssert(type == kValueString); return reinterpret_cast<detail::ValueString const*>(data); }
		inline detail::ValueInt*          InternalGetInt()          { jsonInternalAssert(type == kValueInt);    return reinterpret_cast<detail::ValueInt*>         (data); }
		inline detail::ValueInt const*    InternalGetInt()    const { jsonInternalAssert(type == kValueInt);    return reinterpret_cast<detail::ValueInt const*>   (data); }
		inline detail::ValueFloat*        InternalGetFloat()        { jsonInternalAssert(type == kValueFloat);  return reinterpret_cast<detail::ValueFloat*>       (data); }
		inline detail::ValueFloat const*  InternalGetFloat()  const { jsonInternalAssert(type == kValueFloat);  return reinterpret_cast<detail::ValueFloat const*> (data); }
		inline detail::ValueBool*         InternalGetBool()         { jsonInternalAssert(type == kValueBool);   return reinterpret_cast<detail::ValueBool*>        (data); }
		inline detail::ValueBool const*   InternalGetBool()   const { jsonInternalAssert(type == kValueBool);   return reinterpret_cast<detail::ValueBool const*>  (data); }
		inline detail::ValueArray*        InternalGetArray()        { jsonInternalAssert(type == kValueArray);  return reinterpret_cast<detail::ValueArray*>       (data); }
		inline detail::ValueArray const*  InternalGetArray()  const { jsonInternalAssert(type == kValueArray);  return reinterpret_cast<detail::ValueArray const*> (data); }
		inline detail::ValueObject*       InternalGetObject()       { jsonInternalAssert(type == kValueObject); return reinterpret_cast<detail::ValueObject*>      (data); }
		inline detail::ValueObject const* InternalGetObject() const { jsonInternalAssert(type == kValueObject); return reinterpret_cast<detail::ValueObject const*>(data); }

		template<typename T>
		void DestructValue(void* value)
		{
			T* t = reinterpret_cast<T*>(value);
			t->~T();

			detail::unused_arg(t);
		}

		inline void DestructValue(ValueType type, void* data)
		{
			switch(type)
			{
			case kValueString: DestructValue<detail::ValueString>(data); break;
			case kValueInt:    DestructValue<detail::ValueInt>   (data); break;
			case kValueFloat:  DestructValue<detail::ValueFloat> (data); break;
			case kValueBool:   DestructValue<detail::ValueBool>  (data); break;
			case kValueArray:  DestructValue<detail::ValueArray> (data); break;
			case kValueObject: DestructValue<detail::ValueObject>(data); break;
			case kValueNull: break;//no deleter necessary, nothing constructed into data

			default:
				jsonInternalAssert(!"Invalid type!");
				break;
			}
		}

		template<typename StringT> Value const* GetMemberValue( StringT&& member ) const;
		template<typename StringT> Value*       GetMemberValue( StringT&& member );

		template<typename StringT>
		Value& GetOrAddMember( StringT&& memberName );

		template<typename StringT, typename ValueT>
		Value& GetOrAddMember( StringT&& memberName, ValueT&& value );
	};


	struct Member
	{
		std::string name;
		Value value;

		Member()
		{
		}

		Member( Member const& rhs )
			:name(rhs.name)
			,value(rhs.value)
		{
		}

		Member( Member&& rhs )
			:name(std::move(rhs.name))
			,value(std::move(rhs.value))
		{
		}

		Member& operator=( Member const& rhs )
		{
			name = rhs.name;
			value = rhs.value;
			return *this;
		}

		Member& operator=( Member&& rhs )
		{
			name = std::move(rhs.name);
			value = std::move(rhs.value);
			return *this;
		}

		template<typename StringT, typename ValueT>
		Member( StringT&& name, ValueT&& value )
			:name(std::forward<StringT>(name))
			,value(std::forward<ValueT>(value))
		{
		}
	};


	template<typename StringT>
	bool Value::HasMember( StringT&& memberName ) const
	{
		detail::ValueObject const* obj = InternalGetObject();

		for( std::vector<json::Member>::const_iterator m = obj->members.begin(); m != obj->members.end(); ++m )
		{
			if( m->name == memberName ) return true;
		}
		return false;
	}

	template<typename StringT>
	inline Value const* Value::GetMemberValue( StringT&& memberName ) const
	{
		detail::ValueObject const* obj = InternalGetObject();

		for( std::vector<json::Member>::const_iterator m = obj->members.begin(); m != obj->members.end(); ++m )
		{
			if( m->name == memberName ) return &m->value;
		}

		detail::RaiseException("object does not contain member '" + std::string(memberName) + "'");
		return 0;
	}

	template<typename StringT>
	inline Value* Value::GetMemberValue( StringT&& memberName )
	{
		Value const* value = static_cast<Value const&>(*this).GetMemberValue(memberName);
		return const_cast<Value*>(value);
	}

	template<typename StringT>
	Value& Value::GetOrAddMember( StringT&& memberName )
	{
		return GetOrAddMember(std::forward<StringT>(memberName), Value());
	}

	template<typename StringT, typename ValueT>
	Value& Value::GetOrAddMember( StringT&& memberName, ValueT&& value )
	{
		detail::ValueObject* obj = InternalGetObject();

		for( std::vector<json::Member>::iterator m = obj->members.begin(); m != obj->members.end(); ++m )
		{
			if( m->name == memberName ) return m->value;
		}

		obj->members.push_back( Member(std::forward<StringT>(memberName), std::forward<ValueT>(value)));
		return obj->members.back().value;
	}


	namespace detail
	{
		enum TokenType
		{
			kTokInvalid
			,kTokString
			,kTokNumber
			,kTokOpenBrace    // {
			,kTokCloseBrace   // }
			,kTokColon        // :
			,kTokOpenBracket  // [
			,kTokCloseBracket // ]
			,kTokComma        // ,
			,kTokTrue         // true
			,kTokFalse        // false
			,kTokNull         // null
		};

		inline char const* TokenToString( TokenType type )
		{
			switch( type )
			{
			case kTokInvalid      : return "--kTokInvalid--";
			case kTokString       : return "kTokString";
			case kTokNumber       : return "kTokNumber";
			case kTokOpenBrace    : return "kTokOpenBrace";
			case kTokCloseBrace   : return "kTokCloseBrace";
			case kTokColon        : return "kTokColon";
			case kTokOpenBracket  : return "kTokOpenBracket";
			case kTokCloseBracket : return "kTokCloseBracket";
			case kTokComma        : return "kTokComma";
			case kTokTrue         : return "kTokTrue";
			case kTokFalse        : return "kTokFalse";
			case kTokNull         : return "kTokNull";
			}

			return "<invalid>";
		}


		inline bool TokenHasSymbol( TokenType type )
		{
			switch(type)
			{
			case kTokString:
			case kTokNumber:
				return true;

			default:
				return false;
			}
		}


		struct Token
		{
			TokenType   type;

			std::size_t textBeginIndex;
			std::size_t textLength;

			std::size_t row;
			std::size_t col;

			Token()
			{
				Reset();
			}

			void Reset()
			{
				textBeginIndex = 0;
				textLength = 0;

				row=0;
				col=0;

				type = kTokInvalid;
			}

			std::string GetText( std::vector<char> const& src ) const
			{
				if( textLength > 0 )
				{
					char const* begin( &src[textBeginIndex] );
					return std::string( begin, begin+textLength );
				}
				else
				{
					return std::string();
				}
			}
		};

		typedef std::vector<Token> TokenContainerT;


		struct TokenizerResult
		{
			TokenContainerT tokens;
			bool fault;
			std::string faultMsg;
		};

		void Tokenize( std::vector<char> const& in, TokenizerResult& result );


		struct TokenizerException : std::runtime_error
		{
			explicit TokenizerException(std::string const& message)
				:std::runtime_error(message)
			{
			}

			explicit TokenizerException(char const* message)
				:std::runtime_error(message)
			{
			}
		};

		inline void RaiseTokenizerException( std::size_t row, std::size_t col, char const* message )
		{
			std::stringstream ss;

			ss << "[row: " << row << "][col: " << col << "] : " << message;

			throw TokenizerException( ss.str() );
		}

		struct Tokenizer
		{
			std::vector<char> const& src;
			std::size_t srcCursor;

			TokenContainerT& tokens;

			bool use_char;
			char ch;

			Token token;

			std::size_t row;
			std::size_t col;

			Tokenizer(std::vector<char> const& in, TokenContainerT& tokens)
				:src(in)
				,srcCursor(0)
				,tokens(tokens)
			{
			}

			void Fault( std::string const& message )
			{
				Fault( message.c_str() );
			}

			void Fault( char const* message )
			{
				RaiseTokenizerException(row, col-1, message);
			}

			void Tokenize()
			{
				row = 1;
				col = 1;

				use_char = false;

				for(;;)
				{
					if( use_char && IsWhiteSpace(ch) )
					{
						use_char = false;
					}

					if( use_char || GetChar_SkipWS() )
					{
						use_char = false;

						token.row = row;
						token.col = col-1;

						if     ( ch == '{' ) { token.type = kTokOpenBrace; }
						else if( ch == '}' ) { token.type = kTokCloseBrace; }

						else if( ch == '[' ) { token.type = kTokOpenBracket; }
						else if( ch == ']' ) { token.type = kTokCloseBracket; }

						else if( ch == ':' ) { token.type = kTokColon; }
						else if( ch == ',' ) { token.type = kTokComma; }

						else if( ch == '"' ) { ParseString(); }

						else if( ch == '-' || isdigit(ch) ) { ParseNumber(); }

						else if( ch == 't' ) { ParseConstString("true");  token.type = kTokTrue; }
						else if( ch == 'f' ) { ParseConstString("false"); token.type = kTokFalse; }

						else if( ch == 'n' ) { ParseConstString("null");  token.type = kTokNull; }


						if( token.type == kTokInvalid )
						{
							if( isprint(ch) )
							{
								Fault( std::string("Unrecognized character '") + ch + "'" );
							}
							else
							{
								Fault( std::string("Non-printable character. Binary data?" ) );
							}
						}

						tokens.push_back(token);
						token.Reset();
					}
					else
					{
						return;
					}
				}
			}


			void ParseConstString( std::string const& str )
			{
				std::string s;
				s += ch;

				for(;;)
				{
					//if we're good so far
					int result = str.compare(0, s.size(), s );

					if( result == 0 )
					{
						//are we done?
						if( s.size() == str.size() ) break;

						if( GetChar() )
						{
							s += ch;
						}
						else
						{
							Fault( "Expected string: " + str + ", got EOF" );
						}
					}
					else
					{
						Fault( "Expected string: " + str );
					}
				}
			}

			bool IsWhiteSpace( char ch )
			{
				if( ch == '\n' || ch == '\r' || ch == '\t' || ch == ' ' )
				{
					return true;
				}

				return false;
			}

			void ParseNumber()
			{
				std::size_t numStart = srcCursor-1;
				std::size_t numLen = 1;

				while(GetChar())
				{
					if( isdigit(ch)
						|| ch == '+'
						|| ch == '-'
						|| ch == '.'
						|| ch == 'e'
						|| ch == 'E'
						)
					{
						++numLen;
					}
					else
					{
						use_char = true;
						break;
					}
				}

				token.type = kTokNumber;
				token.textBeginIndex = numStart;
				token.textLength = numLen;
			}


			void ParseString()
			{
				std::size_t strBegin = srcCursor;
				std::size_t strLen   = 0;

				while(GetChar())
				{
					if( ch == '"' )
					{
						//end of string
						break;
					}
					else
					{
						++strLen;
					}
				}

				token.type = kTokString;
				token.textBeginIndex = strBegin;
				token.textLength = strLen;
			}

			bool GetChar()
			{
				if( srcCursor < src.size() )
				{
					ch = src[srcCursor];
					++srcCursor;

					++col;

					if( ch == '\n' )
					{
						++row;
						col = 1;
					}

					return true;
				}

				return false;
			}

			bool GetChar_SkipWS()
			{
				while(GetChar())
				{
					if( IsWhiteSpace(ch) ) continue;
					return true;
				}

				return false;
			}

		private:
			Tokenizer( Tokenizer const& );
			Tokenizer& operator=( Tokenizer const& );
		};

		inline void Tokenize( std::vector<char> const& in, json::detail::TokenizerResult& result )
		{
			try
			{
				Tokenizer tokenizer(in, result.tokens);
				tokenizer.Tokenize();

				result.tokens = tokenizer.tokens;
				result.fault = false;
			}
			catch( TokenizerException const& e )
			{
				result.fault = true;
				result.faultMsg = e.what();
			}
		}
	}

	namespace detail
	{

		struct ParserResult
		{
			bool fault;
			std::string faultMsg;
		};


		struct ParserException : std::runtime_error
		{
			explicit ParserException(std::string const& message)
				:std::runtime_error(message)
			{
			}

			explicit ParserException(char const* message)
				:std::runtime_error(message)
			{
			}
		};

		inline void RaiseParserException( std::vector<char> const& src, Token const* token, char const* message )
		{
			std::stringstream ss;

			if( token )
			{
				ss << "[row: " << token->row << "][col: " << token->col << "][token type '" << token->type << "']";

				if( TokenHasSymbol(token->type) )
				{
					ss << "[text: " << token->GetText(src) << ']';
				}
			}

			ss << " : " << message;

			throw ParserException( ss.str() );
		}



		struct Parser
		{
			std::vector<char> const& src;
			TokenContainerT& tokens;

			Token* token;

			std::size_t pos;
			bool cur_token;


			Parser( std::vector<char> const& src, TokenContainerT& tokens )
				:src(src)
				,tokens(tokens)
				,token(nullptr)
			{
			}

			void Fault( std::string const& message )
			{
				Fault(message.c_str());
			}

			void Fault( char const* message )
			{
				RaiseParserException( src, token, message );
			}

			void Fault( Token const* token, std::string const& message )
			{
				Fault(token, message.c_str());
			}

			void Fault( Token const* token, char const* message )
			{
				RaiseParserException( src, token, message );
			}


			void Parse( Value& root )
			{
				if( tokens.empty() )
				{
					Fault( "No tokens in stream" );
				}

				cur_token = false;
				pos = 0;

				//root should be an object
				ParseObject(root);
			}

			bool NextToken()
			{
				if( cur_token )
				{
					cur_token = false;
					return true;
				}

				if( pos < tokens.size() )
				{
					token = &tokens[pos];
					++pos;
					return true;
				}

				return false;
			}

			bool ExpectTokenType(TokenType type)
			{
				if( NextToken() && token->type == type ) return true;

				return false;
			}

			void ParseObject( Value& obj )
			{
				if( !ExpectTokenType(kTokOpenBrace) ){ Fault("Expected start-of-object"); }

				obj.SetObject();

				ParseObjectMembers(obj);

				if( !ExpectTokenType(kTokCloseBrace) ) { Fault("Expected end-of-object"); }
			}

			void PrefetchTokens( std::size_t num, std::vector<Token const*>& result )
			{
				while( num-- )
				{
					if( NextToken() )
					{
						result.push_back(token);
					}
				}
			}

			void ParseValue( Value& val )
			{
				//figure out what this is

				if( NextToken() )
				{

					switch( token->type )
					{
					case kTokString:
						{
							val.SetString(std::move(token->GetText(src)));
						}
						break;

					case kTokNumber:
						{
							cur_token = true;
							ParseNumber(val);
						}
						break;

					case kTokOpenBrace:
						{
							cur_token = true;
							ParseObject(val);
						}
						break;

					case kTokOpenBracket:
						{
							cur_token = true;
							ParseArray( val );
						}
						break;

					case kTokTrue:
						{
							val.SetBool(true);
						}
						break;

					case kTokFalse:
						{
							val.SetBool(false);
						}
						break;
					case kTokNull:
						{
							val.SetNull();
						}
						break;


					case kTokCloseBrace:
					case kTokColon:
					case kTokCloseBracket:
					case kTokComma:
						//bad!
						Fault("Expected value");
						break;
					}

				}
			}

			void ParseArray( Value& arr )
			{
				if( !ExpectTokenType(kTokOpenBracket) ){ Fault("Expected start-of-array"); }

				arr.SetArray();

				ParseArrayElements(arr);

				if( !ExpectTokenType(kTokCloseBracket) ){ Fault("Expected end-of-array"); }
			}

			void ParseArrayElements( Value& arr )
			{
				for(;;)
				{
					if( NextToken() )
					{
						cur_token = true;

						if( token->type == kTokCloseBracket ) //empty array?
						{
							return;
						}
					}
					else
					{
						Fault("Expected array elements, got EOF ");
					}

					Value element;
					ParseValue(element);

					arr.AddValue(element);

					if( NextToken() )
					{
						if( token->type == kTokComma )
						{
							continue;
						}
						else if( token->type == kTokCloseBracket )
						{
							cur_token = true;
							return;
						}
						else
						{
							Fault("Parsing array elements, expected comma or end-of-array");
							return;
						}
					}
					else
					{
						Fault("Parsing array elements, unexpected EOF ");
						return;
					}

				}
			}

			void ParseNumber( Value& num )
			{
				//figure out if it's an int or float
				if( NextToken() && token->type == kTokNumber )
				{
					std::string text = token->GetText(src);
					bool isfloat = (text.find('.') != std::string::npos);

					std::stringstream ss( std::move(text) );

					if( isfloat )
					{
						//float
						json::float_t f = 0;

						if( ss >> f )
						{
							num.SetFloat( f );
						}
						else
						{
							Fault("Expected float");
						}
					}
					else
					{
						//int
						json::int_t i = 0;

						if( ss >> i )
						{
							num.SetInt( i );
						}
						else
						{
							Fault("Expected int");
						}
					}
				}
				else
				{
					Fault("Expected number, got EOF");
				}
			}

			void ParseObjectMembers( Value& obj )
			{
				for(;;)
				{

					//check for empty object
					if( NextToken() )
					{
						cur_token = true;

						if( token->type == kTokCloseBrace ) //empty object
						{
							return;
						}
					}
					else
					{
						Fault("Expected object member or end of object, got EOF");
					}

					std::vector<Token const*> prefetch;
					PrefetchTokens( 3, prefetch ); // string : value

					if( prefetch.size() == 1 )
					{
						//this is OK, if we hit a close brace (end of object)
						if( prefetch[0]->type == kTokCloseBrace )
						{
							cur_token = true;
						}
						else
						{
							Fault("Expected object member, got EOF");
						}

						return;
					}

					if( prefetch.size() != 3 )
					{
						Fault("Expected object member, got EOF");
						return;
					}

					if( prefetch[0]->type != kTokString )
					{
						Fault( prefetch[0], "Parsing object member, expected member name string");
					}

					if( prefetch[1]->type != kTokColon )
					{
						Fault( prefetch[1], "Parsing object member, expected colon separator");
					}


					Value memberValue;

					cur_token = true;
					ParseValue( memberValue );

					obj.SetValueMember( prefetch[0]->GetText(src), memberValue );

					if( NextToken() && token->type == kTokComma )
					{
						continue;
					}
					else
					{
						//end of the line
						cur_token = true;
						return;
					}
				}
			}

		private:
			Parser( Parser const& );
			Parser& operator=( Parser const& );
		};


		inline ParserResult Parse( std::vector<char> const& src, TokenContainerT& tokens, Value& root )
		{
			ParserResult result;

			try
			{
				Parser parser(src, tokens);
				parser.Parse( root );

				result.fault = false;

			}
			catch( ParserException const& e )
			{
				result.fault = true;
				result.faultMsg = e.what();
			}

			return result;
		}

	}

	inline void read( std::vector<char> const& in, json::Value& root )
	{
		detail::TokenizerResult tokenizerResult;

		detail::Tokenize(in, tokenizerResult);

		if( tokenizerResult.fault )
		{
			detail::RaiseException("Error tokenizing: " + tokenizerResult.faultMsg);
		}

		detail::ParserResult parserResult = detail::Parse( in, tokenizerResult.tokens, root );

		if( parserResult.fault )
		{
			detail::RaiseException("Error parsing: " + parserResult.faultMsg);
		}
	}

	inline void read( std::istream& in, json::Value& root )
	{
		in >> std::noskipws;

		std::vector<char> const inBuff( std::istream_iterator<char>(in), (std::istream_iterator<char>()) );
		read( inBuff, root );
	}

	namespace detail
	{
		namespace json_detail
		{
			static char const* kIndentStr = "\t";
		}

		inline void indent_str( std::ostream& out, std::size_t indent )
		{
			while( indent-- )
			{
				out << json_detail::kIndentStr;
			}
		}

		inline void json_write_impl(std::ostream& out, json::Value const& node, std::size_t indent)
		{
			using namespace json;

			switch(node.type)
			{

			case kValueString:
				{
					out << '"' << node.GetString() << '"';
				}
				break;

			case kValueInt:
				{
					out << node.GetInt();
				}
				break;

			case kValueFloat:
				{
					std::ios_base::fmtflags flags = out.flags();

					out << std::showpoint;
					out << node.GetFloat();

					out.setf(flags);
				}
				break;

			case kValueObject:
				{
					out << "{\n";

					std::size_t numMembers = node.GetNumMembers();

					for( std::size_t memberIdx = 0; memberIdx < numMembers; ++memberIdx )
					{
						Member const& member = node.GetMember(memberIdx);

						indent_str(out, indent+1);



						out << '"' << member.name << "\" : ";

						if( member.value.type == kValueObject )
						{
							out << '\n';
							indent_str(out, indent+2);
						}

						json_write_impl( out, member.value, indent+2 );

						if( memberIdx != numMembers ) out << ",";
						else                 out << " ";

						out << '\n';
					}

					indent_str(out, indent);
					out << "}\n";


				}
				break;

			case kValueArray:
				{
					std::pair<bool, ValueType> arrayInfo = node.IsHomogenousArray();

					bool arrayOfObjects = (arrayInfo.first && arrayInfo.second == kValueObject);

					if( arrayOfObjects )
					{
						//special formatting for this case
						out << '\n';
						indent_str(out, indent);
						out << "[\n";

						std::size_t numElements = node.GetNumElements();

						for( std::size_t e = 0; e < numElements; ++e )
						{
							indent_str(out, indent+1);

							if( e > 0 ) out << ", ";
							else        out << "  ";

							json_write_impl( out, node.GetElement(e), indent+2 );
						}

						indent_str(out, indent);
						out << "]\n";
					}
					else
					{
						out << '[';

						std::size_t numElements = node.GetNumElements();
						for( std::size_t e = 0; e < numElements; ++e )
						{
							if( e > 0 ) out << ", ";
							else        out << " ";

							json_write_impl( out, node.GetElement(e), indent+2 );
						}

						out << " ]";
					}

				}
				break;

			case kValueBool:
				{
					if( node.GetBool() ) out << "true";
					else                 out << "false";
				}
				break;

			case kValueNull:
				{
					out << "null";
				}
				break;
			}
		}

	}

	inline void write( std::ostream& out, json::Value& root )
	{
		//root element must be Object
		if( root.type == kValueObject )
		{
			detail::json_write_impl(out, root, 0);
		}
		else
		{
			detail::RaiseException("root object passed to json::write must be an Object");
		}
	}
}

