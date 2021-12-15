#pragma once
#include "../ItemSerializers.h"
#include "../../../OS/Logging/Logger.h"
#pragma warning(disable: 26819)
#pragma warning(disable: 26495)
#pragma warning(disable: 28020)
#include "nlohmann/json.hpp"
#pragma warning(default: 26819)
#pragma warning(default: 28020)
#pragma warning(default: 26495)


namespace Jimara {
	namespace Serialization {
		/// <summary>
		/// Stores serialized data from a SerializedObject into a json
		/// </summary>
		/// <param name="object"> Serialized object </param>
		/// <param name="logger"> Logger for error/warning reporting </param>
		/// <param name="error"> If error occures, this flag will be set accordingly </param>
		/// <param name="serializerObjectPtr"> 
		///		SerializeToJson is not responsible for interpreting ValueSerializer of any other valid ptr type; this function will be used to fill in the details;
		///		(arguments are: SerializedObject of the object pointer and error)
		/// </param>
		/// <returns> json object </returns>
		nlohmann::json SerializeToJson(const SerializedObject& object, OS::Logger* logger, bool& error,
			const Function<nlohmann::json, const SerializedObject&, bool&>& serializerObjectPtr);

		/// <summary>
		/// Stores serialized data from a SerializedObject into a json
		/// </summary>
		/// <typeparam name="ObjectPtrSerializeCallback"> 
		///		Anything that can be called as a function with (const SerializedObject&, bool&) as arguments as long as it returns a json value
		/// </typeparam>
		/// <param name="object"> Serialized object </param>
		/// <param name="logger"> Logger for error/warning reporting </param>
		/// <param name="error"> If error occures, this flag will be set accordingly </param>
		/// <param name="serializerObjectCallback"> 
		///		SerializeToJson is not responsible for interpreting ValueSerializer of any other valid ptr type; this function will be used to fill in the details;
		///		(arguments are: SerializedObject of the object pointer and error)
		/// </param>
		/// <returns> json object </returns>
		template<typename ObjectPtrSerializeCallback>
		nlohmann::json SerializeToJson(const SerializedObject& object, OS::Logger* logger, bool& error,
			const ObjectPtrSerializeCallback& serializerObjectCallback) {
			nlohmann::json(*callback)(const ObjectPtrSerializeCallback*, const SerializedObject&, bool&) =
				[](const ObjectPtrSerializeCallback* call, const SerializedObject& obj, bool& err) {
				return (*call)(obj, err);
			};
			return SerializeToJson(object, logger, error, Function<nlohmann::json, const SerializedObject&, bool&>(callback, &serializerObjectCallback));
		}

		/// <summary>
		/// Extracts serialized data from a json into a SerializedObject
		/// </summary>
		/// <param name="object"> Serialized object </param>
		/// <param name="json"> Json object to extract data from </param>
		/// <param name="logger"> Logger for error/warning reporting </param>
		/// <param name="deserializerObjectPtr">
		///		DeserializeFromJson is not responsible for interpreting ValueSerializer of any other valid ptr type; this function will be used to fill in the details;
		///		(arguments are: SerializedObject of the object pointer and it's json representation; logic should match that from the corresponding SerializeToJson() call)
		/// </param>
		/// <returns> True, if no error occured (including the one form deserializerObjectPtr call) </returns>
		bool DeserializeFromJson(const SerializedObject& object, const nlohmann::json& json, OS::Logger* logger,
			const Function<bool, const SerializedObject&, const nlohmann::json&>& deserializerObjectPtr);

		/// <summary>
		/// Extracts serialized data from a json into a SerializedObject
		/// </summary>
		/// <typeparam name="ObjectPtrDeserializeCallback"> 
		///		Anything that can be called as a function with (const SerializedObject&, const nlohmann::json&) as arguments as long as it returnes a boolean value
		/// </typeparam>
		/// <param name="object"> Serialized object </param>
		/// <param name="json"> Json object to extract data from </param>
		/// <param name="logger"> Logger for error/warning reporting </param>
		/// <param name="deserializerObjectPtr">
		///		DeserializeFromJson is not responsible for interpreting ValueSerializer of any other valid ptr type; this function will be used to fill in the details;
		///		(arguments are: SerializedObject of the object pointer and it's json representation; logic should match that from the corresponding SerializeToJson() call)
		/// </param>
		/// <returns> True, if no error occured (including the one form deserializerObjectPtr call) </returns>
		template<typename ObjectPtrDeserializeCallback>
		bool DeserializeFromJson(const SerializedObject& object, const nlohmann::json& json, OS::Logger* logger,
			const ObjectPtrDeserializeCallback& deserializerObjectPtr) {
			bool(*callback)(const ObjectPtrDeserializeCallback*, const SerializedObject&, const nlohmann::json&) =
				[](const ObjectPtrDeserializeCallback* call, const SerializedObject& obj, const nlohmann::json& j) {
				return (*call)(obj, j);
			};
			return DeserializeFromJson(object, json, logger, Function<bool, const SerializedObject&, const nlohmann::json&>(callback, &deserializerObjectPtr));
		}
	}
}
