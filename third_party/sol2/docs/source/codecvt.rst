unicode transformation format handling
======================================
because this is surprisingly hard using standard C++
----------------------------------------------------

.. note::

	The ``<codecvt>`` header is no longer used and sol2 now converts utf8, utf16, and utf32 with internal routines. If you have a problem with the transcoding, please `file an issue report`_.

``std::(w)string(u16/u32)`` are assumed to be in the platform's native wide (for ``wstring``) or unicode format. Lua canonically stores its string literals as utf8 and embraces utf8, albeit its storage is simply a sequence of bytes that are also null-terminated (it is also counted and the size is kept around, so embedded nulls can be used in the string). Therefore, if you need to interact with the unicode or wide alternatives of strings, runtime conversions are performed from the (assumed) utf8 string data into other forms. These conversions check for well-formed UTF, and will replace ill-formed characters with the unicode replacement codepoint, 0xFFFD.

Note that we cannot give you a ``string_view`` to utf16 or utf32 strings: Lua does not hold them in memory this way. You can perhaps do your own customization to provide for this if need be. Remember that Lua stores a counted sequence of bytes: serializing your string as bytes and pushing a string type into Lua's stack will work, though do not except any complex string routines or printing to behave nicely with your code.

.. _file an issue report: https://github.com/ThePhD/sol2/issues
