#ifndef __URL_MACROS_H__
#define __URL_MACROS_H__

#define URL_NS_BEGIN namespace URL {
#define URL_NS_END }

#define ReCa reinterpret_cast

#define GET_VAR(name, type) { \
const type* p_##name = ReCa<const type*>(bytes + pos); \
if (!p_##name) { \
if (success) *success = false; \
return replay; \
} \
name = *p_##name; \
pos += sizeof(type); \
}

#define GET_REPLAY_VAR(name, type) { \
type _##name; \
GET_VAR(_##name, type); \
replay->name = _##name; \
}

#define SAVE_VAR(name, size) { \
char _[size] = {}; \
memcpy_s(_, size, &name, size); \
str.append(_, size); \
}

#endif // __URL_MACROS_H__
