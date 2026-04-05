set(CMAKE_CXX_COMPILER "/run/current-system/sw/bin/g++")
set(CMAKE_CXX_COMPILER_ARG1 "")
set(CMAKE_CXX_COMPILER_ID "GNU")
set(CMAKE_CXX_COMPILER_VERSION "14.3.0")
set(CMAKE_CXX_COMPILER_VERSION_INTERNAL "")
set(CMAKE_CXX_COMPILER_WRAPPER "")
set(CMAKE_CXX_STANDARD_COMPUTED_DEFAULT "17")
set(CMAKE_CXX_EXTENSIONS_COMPUTED_DEFAULT "ON")
set(CMAKE_CXX_STANDARD_LATEST "26")
set(CMAKE_CXX_COMPILE_FEATURES "cxx_std_98;cxx_template_template_parameters;cxx_std_11;cxx_alias_templates;cxx_alignas;cxx_alignof;cxx_attributes;cxx_auto_type;cxx_constexpr;cxx_decltype;cxx_decltype_incomplete_return_types;cxx_default_function_template_args;cxx_defaulted_functions;cxx_defaulted_move_initializers;cxx_delegating_constructors;cxx_deleted_functions;cxx_enum_forward_declarations;cxx_explicit_conversions;cxx_extended_friend_declarations;cxx_extern_templates;cxx_final;cxx_func_identifier;cxx_generalized_initializers;cxx_inheriting_constructors;cxx_inline_namespaces;cxx_lambdas;cxx_local_type_template_args;cxx_long_long_type;cxx_noexcept;cxx_nonstatic_member_init;cxx_nullptr;cxx_override;cxx_range_for;cxx_raw_string_literals;cxx_reference_qualified_functions;cxx_right_angle_brackets;cxx_rvalue_references;cxx_sizeof_member;cxx_static_assert;cxx_strong_enums;cxx_thread_local;cxx_trailing_return_types;cxx_unicode_literals;cxx_uniform_initialization;cxx_unrestricted_unions;cxx_user_literals;cxx_variadic_macros;cxx_variadic_templates;cxx_std_14;cxx_aggregate_default_initializers;cxx_attribute_deprecated;cxx_binary_literals;cxx_contextual_conversions;cxx_decltype_auto;cxx_digit_separators;cxx_generic_lambdas;cxx_lambda_init_captures;cxx_relaxed_constexpr;cxx_return_type_deduction;cxx_variable_templates;cxx_std_17;cxx_std_20;cxx_std_23;cxx_std_26")
set(CMAKE_CXX98_COMPILE_FEATURES "cxx_std_98;cxx_template_template_parameters")
set(CMAKE_CXX11_COMPILE_FEATURES "cxx_std_11;cxx_alias_templates;cxx_alignas;cxx_alignof;cxx_attributes;cxx_auto_type;cxx_constexpr;cxx_decltype;cxx_decltype_incomplete_return_types;cxx_default_function_template_args;cxx_defaulted_functions;cxx_defaulted_move_initializers;cxx_delegating_constructors;cxx_deleted_functions;cxx_enum_forward_declarations;cxx_explicit_conversions;cxx_extended_friend_declarations;cxx_extern_templates;cxx_final;cxx_func_identifier;cxx_generalized_initializers;cxx_inheriting_constructors;cxx_inline_namespaces;cxx_lambdas;cxx_local_type_template_args;cxx_long_long_type;cxx_noexcept;cxx_nonstatic_member_init;cxx_nullptr;cxx_override;cxx_range_for;cxx_raw_string_literals;cxx_reference_qualified_functions;cxx_right_angle_brackets;cxx_rvalue_references;cxx_sizeof_member;cxx_static_assert;cxx_strong_enums;cxx_thread_local;cxx_trailing_return_types;cxx_unicode_literals;cxx_uniform_initialization;cxx_unrestricted_unions;cxx_user_literals;cxx_variadic_macros;cxx_variadic_templates")
set(CMAKE_CXX14_COMPILE_FEATURES "cxx_std_14;cxx_aggregate_default_initializers;cxx_attribute_deprecated;cxx_binary_literals;cxx_contextual_conversions;cxx_decltype_auto;cxx_digit_separators;cxx_generic_lambdas;cxx_lambda_init_captures;cxx_relaxed_constexpr;cxx_return_type_deduction;cxx_variable_templates")
set(CMAKE_CXX17_COMPILE_FEATURES "cxx_std_17")
set(CMAKE_CXX20_COMPILE_FEATURES "cxx_std_20")
set(CMAKE_CXX23_COMPILE_FEATURES "cxx_std_23")
set(CMAKE_CXX26_COMPILE_FEATURES "cxx_std_26")

set(CMAKE_CXX_PLATFORM_ID "Linux")
set(CMAKE_CXX_SIMULATE_ID "")
set(CMAKE_CXX_COMPILER_FRONTEND_VARIANT "GNU")
set(CMAKE_CXX_COMPILER_APPLE_SYSROOT "")
set(CMAKE_CXX_SIMULATE_VERSION "")
set(CMAKE_CXX_COMPILER_ARCHITECTURE_ID "x86_64")



set(CMAKE_AR "/run/current-system/sw/bin/ar")
set(CMAKE_CXX_COMPILER_AR "/nix/store/k3qc3y1f6i8g2dgz5z0cf00dj4xc5rrv-gcc-14.3.0/bin/gcc-ar")
set(CMAKE_RANLIB "/run/current-system/sw/bin/ranlib")
set(CMAKE_CXX_COMPILER_RANLIB "/nix/store/k3qc3y1f6i8g2dgz5z0cf00dj4xc5rrv-gcc-14.3.0/bin/gcc-ranlib")
set(CMAKE_LINKER "/run/current-system/sw/bin/ld")
set(CMAKE_LINKER_LINK "")
set(CMAKE_LINKER_LLD "")
set(CMAKE_CXX_COMPILER_LINKER "/nix/store/rinxh4y0akcin90l05j0zr1r3wahl34d-binutils-wrapper-2.44/bin/ld")
set(CMAKE_CXX_COMPILER_LINKER_ID "GNU")
set(CMAKE_CXX_COMPILER_LINKER_VERSION 2.44)
set(CMAKE_CXX_COMPILER_LINKER_FRONTEND_VARIANT GNU)
set(CMAKE_MT "")
set(CMAKE_TAPI "CMAKE_TAPI-NOTFOUND")
set(CMAKE_COMPILER_IS_GNUCXX 1)
set(CMAKE_CXX_COMPILER_LOADED 1)
set(CMAKE_CXX_COMPILER_WORKS TRUE)
set(CMAKE_CXX_ABI_COMPILED TRUE)

set(CMAKE_CXX_COMPILER_ENV_VAR "CXX")

set(CMAKE_CXX_COMPILER_ID_RUN 1)
set(CMAKE_CXX_SOURCE_FILE_EXTENSIONS C;M;c++;cc;cpp;cxx;m;mm;mpp;CPP;ixx;cppm;ccm;cxxm;c++m)
set(CMAKE_CXX_IGNORE_EXTENSIONS inl;h;hpp;HPP;H;o;O;obj;OBJ;def;DEF;rc;RC)

foreach (lang IN ITEMS C OBJC OBJCXX)
  if (CMAKE_${lang}_COMPILER_ID_RUN)
    foreach(extension IN LISTS CMAKE_${lang}_SOURCE_FILE_EXTENSIONS)
      list(REMOVE_ITEM CMAKE_CXX_SOURCE_FILE_EXTENSIONS ${extension})
    endforeach()
  endif()
endforeach()

set(CMAKE_CXX_LINKER_PREFERENCE 30)
set(CMAKE_CXX_LINKER_PREFERENCE_PROPAGATES 1)
set(CMAKE_CXX_LINKER_DEPFILE_SUPPORTED TRUE)
set(CMAKE_LINKER_PUSHPOP_STATE_SUPPORTED TRUE)
set(CMAKE_CXX_LINKER_PUSHPOP_STATE_SUPPORTED TRUE)

# Save compiler ABI information.
set(CMAKE_CXX_SIZEOF_DATA_PTR "8")
set(CMAKE_CXX_COMPILER_ABI "ELF")
set(CMAKE_CXX_BYTE_ORDER "LITTLE_ENDIAN")
set(CMAKE_CXX_LIBRARY_ARCHITECTURE "")

if(CMAKE_CXX_SIZEOF_DATA_PTR)
  set(CMAKE_SIZEOF_VOID_P "${CMAKE_CXX_SIZEOF_DATA_PTR}")
endif()

if(CMAKE_CXX_COMPILER_ABI)
  set(CMAKE_INTERNAL_PLATFORM_ABI "${CMAKE_CXX_COMPILER_ABI}")
endif()

if(CMAKE_CXX_LIBRARY_ARCHITECTURE)
  set(CMAKE_LIBRARY_ARCHITECTURE "")
endif()

set(CMAKE_CXX_CL_SHOWINCLUDES_PREFIX "")
if(CMAKE_CXX_CL_SHOWINCLUDES_PREFIX)
  set(CMAKE_CL_SHOWINCLUDES_PREFIX "${CMAKE_CXX_CL_SHOWINCLUDES_PREFIX}")
endif()





set(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES "/nix/store/jnphbvdlb402x8laviqyhm8gwvf5wfi1-wayland-1.24.0-dev/include;/nix/store/1jhxfvsrrqa21bd26d16q9s5gaz38k2v-libxml2-2.15.1-dev/include;/nix/store/d19sjypqhnagrwjgyqypga1k8ll4ff67-libxslt-1.1.45-dev/include;/nix/store/qk8l5pp8amakl3f40znb2k5akv1yqf54-openssl-3.6.1-dev/include;/nix/store/n9jcqw1isd4laq8n8swxk476fjkilz52-sqlite-3.50.4-dev/include;/nix/store/1ir5yz7lcn4i464pqr9hpjkbs405vch5-zlib-1.3.2-dev/include;/nix/store/2kzxk6lpph443dwd20kdbk7shq93hxj5-libglvnd-1.7.0-dev/include;/nix/store/ky8rr6hskbg7z5c527dz686mvsx9vwm7-vulkan-headers-1.4.328.0/include;/nix/store/5dcc9qrgjgfy79v3zaa339abkpm42bkf-harfbuzz-12.1.0-dev/include;/nix/store/gb1nav24g7xczxc3kkvrh7qp5324aqhv-graphite2-1.3.14-dev/include;/nix/store/wm2l9k84ixsyzh2hrpy686azyjm4yqb1-icu4c-76.1-dev/include;/nix/store/ppy5zqw96vxqp97clvran7r462qbvhrg-libjpeg-turbo-3.1.2-dev/include;/nix/store/wg0p2qk4yqf9q86zi4zqygcwg7x491pg-libpng-apng-1.6.55-dev/include;/nix/store/vz5gd0rv0m2kjca50gacz0zq9qh7i8xf-pcre2-10.46-dev/include;/nix/store/0514l81i9njkvdq51flsa7rc5vprybbb-zstd-1.5.7-dev/include;/nix/store/yxfl4m5220jj28xc7y4vfw42qqscq5zz-libb2-0.98.1/include;/nix/store/kr3m6f34xnzfdwgl6yy9n8j4h13h5yxz-md4c-0.5.2-dev/include;/nix/store/32401bm6rjcwk5nklym07213kp46xyx9-double-conversion-3.3.1-dev/include;/nix/store/x38vbxv9glffanx50z1sg9cqiqsm915s-libproxy-0.5.11-dev/include;/nix/store/0y4fs0ahfp4xy1fg3qr87xrv2g665jr4-dbus-1.14.10-dev/include;/nix/store/wvy4w6azmz68v49f4vhg463h06kp8a52-expat-2.7.4-dev/include;/nix/store/yg38p4p4js1bddhl10d4a92cf7ksf3k1-glib-2.86.3-dev/include;/nix/store/19nl65vb8np9nsd6i61hgrz1w3zbxws2-libffi-3.5.2-dev/include;/nix/store/b6ac1wl4z9vgc7qsawsz6zlf3d6p8xx4-gettext-0.25.1/include;/nix/store/liha2garpdw4q4gd357dmvm7g4ypmxr6-glibc-iconv-2.40/include;/nix/store/qjhxd78pmfibnm2ysaf16j09y20kp52i-unixODBC-2.3.12/include;/nix/store/xbwb7lwqsnjfbqqrj19axl07xr997frs-mariadb-connector-odbc-3.2.6/include;/nix/store/yhanyiybzs5x82l3bwspnf69703yndp3-systemd-258.3-dev/include;/nix/store/85dxkz9g03yxrqni882a9h0aj95s7g4l-util-linux-2.41.3-dev/include;/nix/store/spaa9rd8byg9k0w62cjc0p981g1wj456-mtdev-1.1.7/include;/nix/store/bimg427all6qs624qrs8mk5r3dxz0i6h-lksctp-tools-1.0.21/include;/nix/store/yhb2vy1fx87rw22qm5kwpr7l677qhw9r-libselinux-3.8.1-dev/include;/nix/store/w4395b98xk04k3n3b0va9w54d7h5nssl-libsepol-3.8.1-dev/include;/nix/store/l7qjpl5rrbm4r4kigjibli8qbkpdgrvy-lttng-ust-2.14.0-dev/include;/nix/store/szdlcivnc5l5f5n5n3fd4465jcbrf2s2-liburcu-0.15.5-dev/include;/nix/store/irwrhl39bb13h3j2hh5gi8qkbra9c6jf-libthai-0.1.29-dev/include;/nix/store/x0isq9j2fcsp9bx1zgf544am83qa4yq5-libdrm-2.4.129-dev/include;/nix/store/84wbld9hgfkgn9xpdnicq8ymlzcgyidy-mesa-libgbm-25.1.0/include;/nix/store/srdiazmzcw2z06kxp151x71rglg7fvxs-libdatrie-2019-12-20-dev/include;/nix/store/zpdgfbvfahjwpdmbdiml41299ah250a1-systemd-minimal-libs-258.3-dev/include;/nix/store/qrbwgd09fi7bilk7gx4121sm2cxjs55h-fontconfig-2.17.1-dev/include;/nix/store/59j1dqa03j94z2spyargpyb7qmnrh2jq-freetype-2.13.3-dev/include;/nix/store/5qa1mqy60si4zzlwix5yga3sqj2klnlr-bzip2-1.0.8-dev/include;/nix/store/lkfg58bkmbbb1k4615ccpqdv3y0vk2qj-brotli-1.1.0-dev/include;/nix/store/9m0938zahq7kcfzzix4kkpm8d1iz3nmq-libx11-1.8.12-dev/include;/nix/store/082v1jh8kiyfah8vpw203d7dr8dp94an-xorgproto-2024.1/include;/nix/store/f548r9q4sm59sl8jw3i1n14dwdykfx9b-libxcomposite-0.4.6-dev/include;/nix/store/1qcjly0zswchwwil65b5cxjaywcjh91c-libxfixes-6.0.2-dev/include;/nix/store/v0k5w7pawb7ml41kcgfsg1nirqxxn1w6-libxext-1.3.6-dev/include;/nix/store/jvwb1x0a5x1hhsvzg225r6r8spvl2cf4-libxau-1.0.12-dev/include;/nix/store/p7fh4lkzjd7m6f7kbhf09i2sw2yg5vl1-libxi-1.8.2-dev/include;/nix/store/3rvss3aa0j994jvndf6wbd7llqb6fy3y-libxrender-0.9.12-dev/include;/nix/store/58q0dn2lbm2p04qmds0aymwdd1fr5j67-libxcb-1.17.0-dev/include;/nix/store/ml4cfhhw6af6qq6g3dn7g5j5alrnii88-libxkbcommon-1.11.0-dev/include;/nix/store/2cdrqvd3av1dmxna9xjqv1jccibpvg6m-libxcb-util-0.4.1-dev/include;/nix/store/ia0dh0z2sxw5kx4vcgqbp566c7ia9004-libxcb-image-0.4.1-dev/include;/nix/store/3fcfw014z5i05ay1ag0hfr6p81mb1kzw-libxcb-keysyms-0.4.1-dev/include;/nix/store/a1ppyn9sr288s1ixjx0zr9fi2sjjdlam-libxcb-render-util-0.3.10-dev/include;/nix/store/256alp82fhdgbxx475dp7mk8m29y53rh-libxcb-wm-0.4.2-dev/include;/nix/store/y2n3b6hxv09d8xvd8g2m1nvx6xjbsgj0-libxdmcp-1.1.5-dev/include;/nix/store/fl3mqgw9slfbbkw88lgf5dzfkmv5hzkq-libxtst-1.2.5/include;/nix/store/d6aw2004h90dwlsfcsygzzj4pzm1s31a-libxcb-cursor-0.1.6-dev/include;/nix/store/v8n4dnqcf4nq9qqhzsj8gd3y5fciiahw-libepoxy-1.5.10-dev/include;/nix/store/gzkn1m8ik65579nsrgb2mkq63zyfjcyq-cups-2.4.16-dev/include;/nix/store/z65xq5f8bh9nhv4q9lqcijrndrgclv7z-gmp-with-cxx-6.3.0-dev/include;/nix/store/cqwvfg776qvsy5lf8r8i6agislip69a6-qtbase-6.10.1/include;/nix/store/dqypadqp2s9lsxp7y5pj0q1riljx7f4f-qtlanguageserver-6.10.1/include;/nix/store/xffkmwjw5pnnffj92lh5mar5n9v4g5ba-qtshadertools-6.10.1/include;/nix/store/jl1kwrfxy14822a2czlv51vcdq1cb6yc-qtsvg-6.10.1/include;/nix/store/a2gyv1xdsji6yjwgwb2gy7h43lyy453z-qtdeclarative-6.10.1/include;/nix/store/xzrdmarcyv3rvgf4dxi51583bspjfzz2-qtwayland-6.10.1/include;/nix/store/w37ax8k2r2w2yblmpxsqljfl42sd7jsv-libusb-1.0.29-dev/include;/nix/store/k3qc3y1f6i8g2dgz5z0cf00dj4xc5rrv-gcc-14.3.0/include/c++/14.3.0;/nix/store/k3qc3y1f6i8g2dgz5z0cf00dj4xc5rrv-gcc-14.3.0/include/c++/14.3.0/x86_64-unknown-linux-gnu;/nix/store/k3qc3y1f6i8g2dgz5z0cf00dj4xc5rrv-gcc-14.3.0/include/c++/14.3.0/backward;/nix/store/k3qc3y1f6i8g2dgz5z0cf00dj4xc5rrv-gcc-14.3.0/lib/gcc/x86_64-unknown-linux-gnu/14.3.0/include;/nix/store/k3qc3y1f6i8g2dgz5z0cf00dj4xc5rrv-gcc-14.3.0/include;/nix/store/k3qc3y1f6i8g2dgz5z0cf00dj4xc5rrv-gcc-14.3.0/lib/gcc/x86_64-unknown-linux-gnu/14.3.0/include-fixed;/nix/store/radp574lyk1l05lccg0i4hj4z1p3ig0m-glibc-2.40-218-dev/include")
set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "stdc++;m;gcc_s;gcc;c;gcc_s;gcc")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "/nix/store/hxgv6w0p0hfndfj39n5d6ib8dfhc1ahq-wayland-1.24.0/lib;/nix/store/hrh21452ahlzzyb95gqv749670vyax14-libxml2-2.15.1/lib;/nix/store/ww6ry2jbhk76n3qasx2s1kzam25rznsx-libxslt-1.1.45/lib;/nix/store/j1wa34bbz07lwhgqdzwnj2nr2w4pl7zr-openssl-3.6.1/lib;/nix/store/77mp6vj6q35gaq2k11ar77q45jnvxfh7-sqlite-3.50.4/lib;/nix/store/3j0jwcygamiza2n1ga69sdk6skaz5k3k-zlib-1.3.2/lib;/nix/store/44iwa5fdrx4hgzf3l9j9q9y5zffwgigy-libglvnd-1.7.0/lib;/nix/store/4p6k6dj0pic7mvr6my01wdkfx9h40hj4-vulkan-loader-1.4.328.0/lib;/nix/store/d9rn1s5zi1sgdaqqf4xqvczrmg1s4cvb-graphite2-1.3.14/lib;/nix/store/yzyh44g07mb5d3151lvv0ll8rdmg1fbl-harfbuzz-12.1.0/lib;/nix/store/sl7j5z4lij096xayzgwiv20p7crx2h5j-icu4c-76.1/lib;/nix/store/8y7g3qd1n7shk3sd0gdg654fy31918h9-libjpeg-turbo-3.1.2/lib;/nix/store/09wbn9wfigxq0lf95wr8naidwlhg1chf-libpng-apng-1.6.55/lib;/nix/store/zizda6ynmi8w6zryk8i9cfh6xa3rwqfz-pcre2-10.46/lib;/nix/store/bnnzrqcdix96yliwz2l2a4vphhg30nzc-zstd-1.5.7/lib;/nix/store/yxfl4m5220jj28xc7y4vfw42qqscq5zz-libb2-0.98.1/lib;/nix/store/s7j6ldh3ra79p2gxxb8hrc8c0w896mky-md4c-0.5.2-lib/lib;/nix/store/kmb3gcijmv3j1gqp24j0q1liwrhb03z3-double-conversion-3.3.1/lib;/nix/store/y8kqz2j7syx5gkzzhjnjpv4axm5b0r88-libproxy-0.5.11/lib;/nix/store/av7gx5cl22ijzm8rx9hn7gprknvq5xxa-expat-2.7.4/lib;/nix/store/2ikmh7bqngbjkkrmhwdb5yld6akncnd5-dbus-1.14.10-lib/lib;/nix/store/wxm6pczq28ppr7ffwclsl6njbzzr48zf-libffi-3.5.2/lib;/nix/store/b6ac1wl4z9vgc7qsawsz6zlf3d6p8xx4-gettext-0.25.1/lib;/nix/store/mg151w65chcirlr7398jrz916wrfh3gg-glib-2.86.3/lib;/nix/store/qjhxd78pmfibnm2ysaf16j09y20kp52i-unixODBC-2.3.12/lib;/nix/store/9xiw907ghrw8an1viscrykykw2bjq9mq-sqlite-connector-odbc-0.9993/lib;/nix/store/bigkpra9jw48fip69q4wndsf4kb3d2w9-systemd-258.3/lib;/nix/store/0dacpbkshmpsr7n7mx0l43c7wgj1gn4v-util-linux-2.41.3-lib/lib;/nix/store/spaa9rd8byg9k0w62cjc0p981g1wj456-mtdev-1.1.7/lib;/nix/store/bimg427all6qs624qrs8mk5r3dxz0i6h-lksctp-tools-1.0.21/lib;/nix/store/qw4fdvxbb87hkkmiq25jkp0s3ls7c70a-libselinux-3.8.1/lib;/nix/store/43h42yan7mijadnj915rb2nma2k7kp0q-libsepol-3.8.1/lib;/nix/store/y93lhgbw0vqg6946is7dbiai2bxlnspr-liburcu-0.15.5/lib;/nix/store/ww04whq3yipazwwbwvq2lhq5z1xps1jw-lttng-ust-2.14.0/lib;/nix/store/m0wzfasq5xm8s3rm5ay8dq4zxr1mf9nc-libthai-0.1.29/lib;/nix/store/hv9q1rr4rnfnnvc84ww2achz9g6sgh6w-libdrm-2.4.129/lib;/nix/store/84wbld9hgfkgn9xpdnicq8ymlzcgyidy-mesa-libgbm-25.1.0/lib;/nix/store/azfygpjykh0cxjji1ya209h3g7fij6s3-libdatrie-2019-12-20-lib/lib;/nix/store/zc8xw2hmch7sq82bz20xrrqf2fibgwcz-systemd-minimal-libs-258.3/lib;/nix/store/qbp6607qrl508sdg9dnkzr5avhk54w48-bzip2-1.0.8/lib;/nix/store/rsmb0b09rjzrjhng3wcqcx7bab9kjjik-brotli-1.1.0-lib/lib;/nix/store/n8d8ch4ify4rbmyyvgzcqnpr0qak7b2s-freetype-2.13.3/lib;/nix/store/vry1ic8kdpmw42mp3qphjnwdh7n946gb-fontconfig-2.17.1-lib/lib;/nix/store/0d2nplzyyigdjbd9l7s1ka4809zm7pwl-libx11-1.8.12/lib;/nix/store/1ppwc4g683dq8d7yq9ybfc7g1l7fh93f-libxfixes-6.0.2/lib;/nix/store/q5fkrrlh2xndwnfbfwx17vvx3a72gndc-libxcomposite-0.4.6/lib;/nix/store/hzpbah5kw3pv1nfglw8pnbbfr1xvcdj5-libxau-1.0.12/lib;/nix/store/x0pzlm34sbaz7kshyj6889lqihs368gd-libxext-1.3.6/lib;/nix/store/480scknkmmianwsrd0arm8z0fhssh4hn-libxi-1.8.2/lib;/nix/store/b54y6m5pkp44zgzv8rhkv40aq2rk0gkq-libxrender-0.9.12/lib;/nix/store/4jp9hx8spdys5g8rvadp238y4z70nv0a-libxcb-1.17.0/lib;/nix/store/yl2zgiqvwxf8ann2nfqvzwv4xj01cv8h-libxkbcommon-1.11.0/lib;/nix/store/ll3wdm9lm6qmdfbifxqjsqzlvjia71ng-libxcb-util-0.4.1/lib;/nix/store/yaaxw21x93v9ccgzbwbq57626xq8q2ha-libxcb-image-0.4.1/lib;/nix/store/7c5zf9xa8qxn5fzbc4vvsj3b9vjah9xb-libxcb-keysyms-0.4.1/lib;/nix/store/v7lp5464r5914ilk9rv9n641i49f5cjd-libxcb-render-util-0.3.10/lib;/nix/store/7467y7gsd6lif9335dgkc6hlz8vqaliz-libxcb-wm-0.4.2/lib;/nix/store/gx399prl1npnz6cb74ki6x5vbd5dazv5-libxdmcp-1.1.5/lib;/nix/store/fl3mqgw9slfbbkw88lgf5dzfkmv5hzkq-libxtst-1.2.5/lib;/nix/store/5v516g9r3xdc5ia2azf28dw64aa1cgzr-libxcb-cursor-0.1.6/lib;/nix/store/k97vkxlby20zyn4l4x6xygcv74dn7fji-libepoxy-1.5.10/lib;/nix/store/km81slwkcc82dbwywl10gpffjb78g6ni-gmp-with-cxx-6.3.0/lib;/nix/store/g2fb201c3iaasabfl0jg41nbxgly7wm4-cups-2.4.16-lib/lib;/nix/store/cqwvfg776qvsy5lf8r8i6agislip69a6-qtbase-6.10.1/lib;/nix/store/dqypadqp2s9lsxp7y5pj0q1riljx7f4f-qtlanguageserver-6.10.1/lib;/nix/store/xffkmwjw5pnnffj92lh5mar5n9v4g5ba-qtshadertools-6.10.1/lib;/nix/store/jl1kwrfxy14822a2czlv51vcdq1cb6yc-qtsvg-6.10.1/lib;/nix/store/a2gyv1xdsji6yjwgwb2gy7h43lyy453z-qtdeclarative-6.10.1/lib;/nix/store/xzrdmarcyv3rvgf4dxi51583bspjfzz2-qtwayland-6.10.1/lib;/nix/store/q0rms8inhvb30vhfsaq89krqckvx0zny-libusb-1.0.29/lib;/nix/store/km4g87jxsqxvcq344ncyb8h1i6f3cqxh-glibc-2.40-218/lib;/nix/store/k3qc3y1f6i8g2dgz5z0cf00dj4xc5rrv-gcc-14.3.0/lib/gcc/x86_64-unknown-linux-gnu/14.3.0;/nix/store/alrbhz7im0w0jdwcfdgcfk7pxhkl1fzj-gcc-14.3.0-lib/lib;/nix/store/k3qc3y1f6i8g2dgz5z0cf00dj4xc5rrv-gcc-14.3.0/lib")
set(CMAKE_CXX_IMPLICIT_LINK_FRAMEWORK_DIRECTORIES "")
set(CMAKE_CXX_COMPILER_CLANG_RESOURCE_DIR "")

set(CMAKE_CXX_COMPILER_IMPORT_STD "")
### Imported target for C++23 standard library
set(CMAKE_CXX23_COMPILER_IMPORT_STD_NOT_FOUND_MESSAGE "Unsupported generator: Unix Makefiles")


### Imported target for C++26 standard library
set(CMAKE_CXX26_COMPILER_IMPORT_STD_NOT_FOUND_MESSAGE "Unsupported generator: Unix Makefiles")



