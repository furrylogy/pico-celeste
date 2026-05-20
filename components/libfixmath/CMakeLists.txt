idf_component_register(SRCS "fix16.c" "fix16_exp.c" "fix16_sqrt.c" "fix16_str.c" "fix16_trig.c" "fract32.c" "uint32.c"
                       INCLUDE_DIRS ".")

target_compile_definitions(${COMPONENT_LIB} PRIVATE FIXMATH_NO_CACHE)