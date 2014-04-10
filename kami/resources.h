#pragma once

#include <memory>       //  We need std::unique_ptr
#include <stdexcept>    //  We need std::runtime_error
#include <cassert>      //  We need assert()
#include <map>

namespace klib {
	template <typename ResourceType, typename Identifier>
	class Resource {
	public:
		void load(Identifier id, const std::string& filename){
			//  Create a pointer to a new Resource object.
			std::unique_ptr<ResourceType> resource(new ResourceType(filename.c_str()));

			//  If the loading was successful, insert the resource into the map.
			auto inserted = mResourceMap.insert(std::make_pair(id, std::move(resource)));
			//  Inserting into a map returns a pair of values: an iterator and a boolean, respectively.
			//  If the insertion worked, the boolean will be true. We check this with assert.
			assert(inserted.second);
		}

		ResourceType&       get(Identifier id){
			//  Search the map for the id.
			auto found = mResourceMap.find(id);
			//  If it is not found, it will reference end().
			//  If this is the case, the program will halt if
			//  it is in debug mode.
			assert(found != mResourceMap.end());
			//  Return the.. something.
			return *found->second.get();
			//  *(*found).second;   Return the value pointed to by the value pointed to by the found iterator.
		};

		const ResourceType& get(Identifier id) const {
			auto found = mResourceMap.find(id);
			assert(found != mResourceMap.end());
			return const_cast<const ResourceType>(*found->second.get());
		};

		ResourceType& operator[](Identifier id){
			return get(id);
		};

		void operator()(Identifier id, const std::string& filename){
			load(id, filename);
		}

	private:
		std::map<Identifier, std::unique_ptr<ResourceType>> mResourceMap;

		void insertResource(Identifier id, std::unique_ptr<ResourceType> resource){
			//  Insert element and check success.
			auto inserted = mResourceMap.insert(std::make_pair(id, std::move(resource)));
			assert(inserted.second);
		};
	};
}