        module dag_struct
        use ISO_C_BINDING
        type,bind(C) :: ptr_array
        integer(c_int) len
        type (c_ptr) :: arr
        end type  ptr_array
        end module dag_struct
