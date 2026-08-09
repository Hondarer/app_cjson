/* cJSON の公開関数一覧。
 *
 * このヘッダーは意図的に include guard を持たない。インクルード元で
 * MOCK_CJSON_RET と MOCK_CJSON_VOID を定義し、宣言、実装、テストを同じ一覧から生成する。
 */

#ifndef MOCK_CJSON_RET
    #error MOCK_CJSON_RET must be defined before including mock_cjson_api_table.h
#endif
#ifndef MOCK_CJSON_VOID
    #error MOCK_CJSON_VOID must be defined before including mock_cjson_api_table.h
#endif

/* cJSON.h */
MOCK_CJSON_RET(const char *, cJSON_Version, (), (), ())
MOCK_CJSON_VOID(void, cJSON_InitHooks, (cJSON_Hooks * hooks), (hooks), (_))
MOCK_CJSON_RET(cJSON *, cJSON_Parse, (const char *value), (value), (_))
MOCK_CJSON_RET(cJSON *, cJSON_ParseWithLength, (const char *value, size_t buffer_length), (value, buffer_length),
               (_, _))
MOCK_CJSON_RET(cJSON *, cJSON_ParseWithOpts,
               (const char *value, const char **return_parse_end, cJSON_bool require_null_terminated),
               (value, return_parse_end, require_null_terminated), (_, _, _))
MOCK_CJSON_RET(cJSON *, cJSON_ParseWithLengthOpts,
               (const char *value, size_t buffer_length, const char **return_parse_end,
                cJSON_bool require_null_terminated),
               (value, buffer_length, return_parse_end, require_null_terminated), (_, _, _, _))
MOCK_CJSON_RET(char *, cJSON_Print, (const cJSON *item), (item), (_))
MOCK_CJSON_RET(char *, cJSON_PrintUnformatted, (const cJSON *item), (item), (_))
MOCK_CJSON_RET(char *, cJSON_PrintBuffered, (const cJSON *item, int prebuffer, cJSON_bool fmt), (item, prebuffer, fmt),
               (_, _, _))
MOCK_CJSON_RET(cJSON_bool, cJSON_PrintPreallocated,
               (cJSON * item, char *buffer, const int length, const cJSON_bool format), (item, buffer, length, format),
               (_, _, _, _))
MOCK_CJSON_VOID(void, cJSON_Delete, (cJSON * item), (item), (_))
MOCK_CJSON_RET(int, cJSON_GetArraySize, (const cJSON *array), (array), (_))
MOCK_CJSON_RET(cJSON *, cJSON_GetArrayItem, (const cJSON *array, int index), (array, index), (_, _))
MOCK_CJSON_RET(cJSON *, cJSON_GetObjectItem, (const cJSON *const object, const char *const string), (object, string),
               (_, _))
MOCK_CJSON_RET(cJSON *, cJSON_GetObjectItemCaseSensitive, (const cJSON *const object, const char *const string),
               (object, string), (_, _))
MOCK_CJSON_RET(cJSON_bool, cJSON_HasObjectItem, (const cJSON *object, const char *string), (object, string), (_, _))
MOCK_CJSON_RET(const char *, cJSON_GetErrorPtr, (), (), ())
MOCK_CJSON_RET(char *, cJSON_GetStringValue, (const cJSON *const item), (item), (_))
MOCK_CJSON_RET(double, cJSON_GetNumberValue, (const cJSON *const item), (item), (_))
MOCK_CJSON_RET(cJSON_bool, cJSON_IsInvalid, (const cJSON *const item), (item), (_))
MOCK_CJSON_RET(cJSON_bool, cJSON_IsFalse, (const cJSON *const item), (item), (_))
MOCK_CJSON_RET(cJSON_bool, cJSON_IsTrue, (const cJSON *const item), (item), (_))
MOCK_CJSON_RET(cJSON_bool, cJSON_IsBool, (const cJSON *const item), (item), (_))
MOCK_CJSON_RET(cJSON_bool, cJSON_IsNull, (const cJSON *const item), (item), (_))
MOCK_CJSON_RET(cJSON_bool, cJSON_IsNumber, (const cJSON *const item), (item), (_))
MOCK_CJSON_RET(cJSON_bool, cJSON_IsString, (const cJSON *const item), (item), (_))
MOCK_CJSON_RET(cJSON_bool, cJSON_IsArray, (const cJSON *const item), (item), (_))
MOCK_CJSON_RET(cJSON_bool, cJSON_IsObject, (const cJSON *const item), (item), (_))
MOCK_CJSON_RET(cJSON_bool, cJSON_IsRaw, (const cJSON *const item), (item), (_))
MOCK_CJSON_RET(cJSON *, cJSON_CreateNull, (), (), ())
MOCK_CJSON_RET(cJSON *, cJSON_CreateTrue, (), (), ())
MOCK_CJSON_RET(cJSON *, cJSON_CreateFalse, (), (), ())
MOCK_CJSON_RET(cJSON *, cJSON_CreateBool, (cJSON_bool boolean), (boolean), (_))
MOCK_CJSON_RET(cJSON *, cJSON_CreateNumber, (double num), (num), (_))
MOCK_CJSON_RET(cJSON *, cJSON_CreateString, (const char *string), (string), (_))
MOCK_CJSON_RET(cJSON *, cJSON_CreateRaw, (const char *raw), (raw), (_))
MOCK_CJSON_RET(cJSON *, cJSON_CreateArray, (), (), ())
MOCK_CJSON_RET(cJSON *, cJSON_CreateObject, (), (), ())
MOCK_CJSON_RET(cJSON *, cJSON_CreateStringReference, (const char *string), (string), (_))
MOCK_CJSON_RET(cJSON *, cJSON_CreateObjectReference, (const cJSON *child), (child), (_))
MOCK_CJSON_RET(cJSON *, cJSON_CreateArrayReference, (const cJSON *child), (child), (_))
MOCK_CJSON_RET(cJSON *, cJSON_CreateIntArray, (const int *numbers, int count), (numbers, count), (_, _))
MOCK_CJSON_RET(cJSON *, cJSON_CreateFloatArray, (const float *numbers, int count), (numbers, count), (_, _))
MOCK_CJSON_RET(cJSON *, cJSON_CreateDoubleArray, (const double *numbers, int count), (numbers, count), (_, _))
MOCK_CJSON_RET(cJSON *, cJSON_CreateStringArray, (const char *const *strings, int count), (strings, count), (_, _))
MOCK_CJSON_RET(cJSON_bool, cJSON_AddItemToArray, (cJSON * array, cJSON *item), (array, item), (_, _))
MOCK_CJSON_RET(cJSON_bool, cJSON_AddItemToObject, (cJSON * object, const char *string, cJSON *item),
               (object, string, item), (_, _, _))
MOCK_CJSON_RET(cJSON_bool, cJSON_AddItemToObjectCS, (cJSON * object, const char *string, cJSON *item),
               (object, string, item), (_, _, _))
MOCK_CJSON_RET(cJSON_bool, cJSON_AddItemReferenceToArray, (cJSON * array, cJSON *item), (array, item), (_, _))
MOCK_CJSON_RET(cJSON_bool, cJSON_AddItemReferenceToObject, (cJSON * object, const char *string, cJSON *item),
               (object, string, item), (_, _, _))
MOCK_CJSON_RET(cJSON *, cJSON_DetachItemViaPointer, (cJSON * parent, cJSON *const item), (parent, item), (_, _))
MOCK_CJSON_RET(cJSON *, cJSON_DetachItemFromArray, (cJSON * array, int which), (array, which), (_, _))
MOCK_CJSON_VOID(void, cJSON_DeleteItemFromArray, (cJSON * array, int which), (array, which), (_, _))
MOCK_CJSON_RET(cJSON *, cJSON_DetachItemFromObject, (cJSON * object, const char *string), (object, string), (_, _))
MOCK_CJSON_RET(cJSON *, cJSON_DetachItemFromObjectCaseSensitive, (cJSON * object, const char *string), (object, string),
               (_, _))
MOCK_CJSON_VOID(void, cJSON_DeleteItemFromObject, (cJSON * object, const char *string), (object, string), (_, _))
MOCK_CJSON_VOID(void, cJSON_DeleteItemFromObjectCaseSensitive, (cJSON * object, const char *string), (object, string),
                (_, _))
MOCK_CJSON_RET(cJSON_bool, cJSON_InsertItemInArray, (cJSON * array, int which, cJSON *newitem), (array, which, newitem),
               (_, _, _))
MOCK_CJSON_RET(cJSON_bool, cJSON_ReplaceItemViaPointer, (cJSON *const parent, cJSON *const item, cJSON *replacement),
               (parent, item, replacement), (_, _, _))
MOCK_CJSON_RET(cJSON_bool, cJSON_ReplaceItemInArray, (cJSON * array, int which, cJSON *newitem),
               (array, which, newitem), (_, _, _))
MOCK_CJSON_RET(cJSON_bool, cJSON_ReplaceItemInObject, (cJSON * object, const char *string, cJSON *newitem),
               (object, string, newitem), (_, _, _))
MOCK_CJSON_RET(cJSON_bool, cJSON_ReplaceItemInObjectCaseSensitive, (cJSON * object, const char *string, cJSON *newitem),
               (object, string, newitem), (_, _, _))
MOCK_CJSON_RET(cJSON *, cJSON_Duplicate, (const cJSON *item, cJSON_bool recurse), (item, recurse), (_, _))
MOCK_CJSON_RET(cJSON_bool, cJSON_Compare, (const cJSON *const a, const cJSON *const b, const cJSON_bool case_sensitive),
               (a, b, case_sensitive), (_, _, _))
MOCK_CJSON_VOID(void, cJSON_Minify, (char *json), (json), (_))
MOCK_CJSON_RET(cJSON *, cJSON_AddNullToObject, (cJSON *const object, const char *const name), (object, name), (_, _))
MOCK_CJSON_RET(cJSON *, cJSON_AddTrueToObject, (cJSON *const object, const char *const name), (object, name), (_, _))
MOCK_CJSON_RET(cJSON *, cJSON_AddFalseToObject, (cJSON *const object, const char *const name), (object, name), (_, _))
MOCK_CJSON_RET(cJSON *, cJSON_AddBoolToObject, (cJSON *const object, const char *const name, const cJSON_bool boolean),
               (object, name, boolean), (_, _, _))
MOCK_CJSON_RET(cJSON *, cJSON_AddNumberToObject, (cJSON *const object, const char *const name, const double number),
               (object, name, number), (_, _, _))
MOCK_CJSON_RET(cJSON *, cJSON_AddStringToObject,
               (cJSON *const object, const char *const name, const char *const string), (object, name, string),
               (_, _, _))
MOCK_CJSON_RET(cJSON *, cJSON_AddRawToObject, (cJSON *const object, const char *const name, const char *const raw),
               (object, name, raw), (_, _, _))
MOCK_CJSON_RET(cJSON *, cJSON_AddObjectToObject, (cJSON *const object, const char *const name), (object, name), (_, _))
MOCK_CJSON_RET(cJSON *, cJSON_AddArrayToObject, (cJSON *const object, const char *const name), (object, name), (_, _))
MOCK_CJSON_RET(double, cJSON_SetNumberHelper, (cJSON * object, double number), (object, number), (_, _))
MOCK_CJSON_RET(char *, cJSON_SetValuestring, (cJSON * object, const char *valuestring), (object, valuestring), (_, _))
MOCK_CJSON_RET(void *, cJSON_malloc, (size_t size), (size), (_))
MOCK_CJSON_VOID(void, cJSON_free, (void *object), (object), (_))

/* cJSON_Utils.h */
MOCK_CJSON_RET(cJSON *, cJSONUtils_GetPointer, (cJSON *const object, const char *pointer), (object, pointer), (_, _))
MOCK_CJSON_RET(cJSON *, cJSONUtils_GetPointerCaseSensitive, (cJSON *const object, const char *pointer),
               (object, pointer), (_, _))
MOCK_CJSON_RET(cJSON *, cJSONUtils_GeneratePatches, (cJSON *const from, cJSON *const to), (from, to), (_, _))
MOCK_CJSON_RET(cJSON *, cJSONUtils_GeneratePatchesCaseSensitive, (cJSON *const from, cJSON *const to), (from, to),
               (_, _))
MOCK_CJSON_VOID(void, cJSONUtils_AddPatchToArray,
                (cJSON *const array, const char *const operation, const char *const path, const cJSON *const value),
                (array, operation, path, value), (_, _, _, _))
MOCK_CJSON_RET(int, cJSONUtils_ApplyPatches, (cJSON *const object, const cJSON *const patches), (object, patches),
               (_, _))
MOCK_CJSON_RET(int, cJSONUtils_ApplyPatchesCaseSensitive, (cJSON *const object, const cJSON *const patches),
               (object, patches), (_, _))
MOCK_CJSON_RET(cJSON *, cJSONUtils_MergePatch, (cJSON * target, const cJSON *const patch), (target, patch), (_, _))
MOCK_CJSON_RET(cJSON *, cJSONUtils_MergePatchCaseSensitive, (cJSON * target, const cJSON *const patch), (target, patch),
               (_, _))
MOCK_CJSON_RET(cJSON *, cJSONUtils_GenerateMergePatch, (cJSON *const from, cJSON *const to), (from, to), (_, _))
MOCK_CJSON_RET(cJSON *, cJSONUtils_GenerateMergePatchCaseSensitive, (cJSON *const from, cJSON *const to), (from, to),
               (_, _))
MOCK_CJSON_RET(char *, cJSONUtils_FindPointerFromObjectTo, (const cJSON *const object, const cJSON *const target),
               (object, target), (_, _))
MOCK_CJSON_VOID(void, cJSONUtils_SortObject, (cJSON *const object), (object), (_))
MOCK_CJSON_VOID(void, cJSONUtils_SortObjectCaseSensitive, (cJSON *const object), (object), (_))
