# First-party warning policy. Vendored dependencies intentionally do not use
# this function because upstream warning debt must not hide project warnings.
function(venom_apply_warnings target)
  if(NOT VENOM_ENABLE_STRICT)
    return()
  endif()

  if(MSVC)
    target_compile_options(${target} PRIVATE
      /W4
      /permissive-
      /Zc:preprocessor
    )
    if(VENOM_ENABLE_WERROR)
      target_compile_options(${target} PRIVATE /WX)
    endif()
  else()
    target_compile_options(${target} PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wshadow
      -Wformat=2
      -Wundef
    )
    if(VENOM_ENABLE_WERROR)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  endif()
endfunction()
