#ifndef __TYPECHECK_H__
#define __TYPECHECK_H__

/*在编译时检测类型是否相同*/
#define typecheck(type, x) \
({  \
    type __dummy; \
    typeof(x) __dummy2; \
    (void)(&__dummy == &__dummy2); \
    1; \
})

#endif
