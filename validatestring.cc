/*

Copyright (C) 2018-2018 Gene Harvey

This file is part of Octave.

Octave is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Octave is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Octave; see the file COPYING.  If not, see
<https://www.gnu.org/licenses/>.

*/

#include <octave/oct.h>
#include <octave/oct-string.h>

DEFUN_DLD (validatestring, args, nargout, 
"-*- texinfo -*-\n\
@deftypefn  {} {@var{validstr} =} validatestring (@var{str}, @var{strarray})\n\
@deftypefnx {} {@var{validstr} =} validatestring (@var{str}, @var{strarray}, @var{funcname})\n\
@deftypefnx {} {@var{validstr} =} validatestring (@var{str}, @var{strarray}, @var{funcname}, @var{varname})\n\
@deftypefnx {} {@var{validstr} =} validatestring (@dots{}, @var{position})\n\
Verify that @var{str} is an element, or substring of an element, in\n\
@var{strarray}.\n\
\n\
When @var{str} is a character string to be tested, and @var{strarray} is a\n\
cellstr of valid values, then @var{validstr} will be the validated form\n\
of @var{str} where validation is defined as @var{str} being a member\n\
or substring of @var{validstr}.  This is useful for both verifying\n\
and expanding short options, such as @qcode{\"r\"}, to their longer forms,\n\
such as @qcode{\"red\"}.  If @var{str} is a substring of @var{validstr}, and\n\
there are multiple matches, the shortest match will be returned if all\n\
matches are substrings of each other.  Otherwise, an error will be raised\n\
because the expansion of @var{str} is ambiguous.  All comparisons are case\n\
insensitive.\n\
\n\
The additional inputs @var{funcname}, @var{varname}, and @var{position}\n\
are optional and will make any generated validation error message more\n\
specific.\n\
\n\
Examples:\n\
@c Set example in small font to prevent overfull line\n\
\n\
@smallexample\n\
@group\n\
validatestring (\"r\", @{\"red\", \"green\", \"blue\"@})\n\
@result{} \"red\"\n\
\n\
validatestring (\"b\", @{\"red\", \"green\", \"blue\", \"black\"@})\n\
@result{} error: validatestring: multiple unique matches were found for 'b':\n\
   blue, black\n\
@end group\n\
@end smallexample\n\
\n\
@seealso{strcmp, strcmpi, validateattributes, inputParser}\n\
@end deftypefn ")
{
  octave_idx_type          i;
  std::string              errstr;
  std::string              non_match_str;
  
  octave_value             ov_str;
  octave_value             ov_strarray;
  octave_value             ov_funcname;
  octave_value             ov_varname;
  
  std::string        str;
  Array<std::string> strarray;
  std::string        funcname;
  std::string        varname;
  
  octave_idx_type          num_strs, nmatches;
  Array<octave_idx_type>   matches;
  
  octave_idx_type          len, min_len, min_len_idx;
  
  octave_idx_type          nstr     = 0;
  octave_idx_type          nargin   = args.length ();
  octave_idx_type          position = 0;
  
  if (nargin < 2 || nargin > 5)
  {
    print_usage ();
    return octave_value ();
  }
  
  ov_str      = args(0);
  ov_strarray = args(1);
  
  for (i = 2; i < nargin; i++)
  {
    if (args(i).is_string ())
    {
      switch (nstr)
      {
        case 0:  ov_funcname = args(i); nstr++; break;
        case 1:  ov_varname  = args(i); nstr++; break;
        default: error ("validatestring: invalid number of character inputs (3)");
      }
    }
  }
  
  if (nstr > 2)
  {
    error (("validatestring: invalid number of character inputs (" + std::to_string(nstr) + ")").c_str ());
  }
  
  if (nargin > 2 && args(nargin - 1).isnumeric ())
  {
    position = (args(nargin - 1).fix ()).idx_type_value ();
  }
  
  if (! ov_str.is_string ())
  {
    error ("validatestring: STR must be a character string");
  }
  else if (ov_str.ndims () != 2 || ov_str.dims ()(0) != 1)
  {
    error ("validatestring: STR must be a single row vector");
  }
  else if (ov_strarray.isempty ())
  {
    error ("validatestring: STRARRAY must be non-empty");
  }
  else if (! ov_strarray.iscellstr ())
  {
    error ("validatestring: STRARRAY must be a cellstr");
  }
  else if (! ov_funcname.isempty () && (ov_funcname.ndims () != 2 || ov_funcname.dims ()(0) != 1))
  {
    error ("validatestring: FUNCNAME must be a single row vector");
  }
  else if (! ov_varname.isempty () && (ov_varname.ndims () != 2 || ov_varname.dims ()(0) != 1))
  {
    error ("validatestring: VARNAME must be a single row vector");
  }
  else if (position < 0)
  {
     error ("validatestring: POSITION must be >= 0");
  }
  
  str      = ov_str.string_value ();
  strarray = ov_strarray.cellstr_value ();
  
  if (! ov_funcname.isempty ())
  {
    funcname = ov_funcname.string_value ();
    errstr   = funcname + ": ";
  }
  
  if (! ov_varname.isempty ())
  {
    varname =  ov_varname.string_value ();
    errstr  += varname + " ";
  }
  else
  {
    errstr += "'" + str + "' ";
  }
  
  if (position > 0)
  {
    errstr += "(argument #" + std::to_string(position) + ") ";
  }
  
  num_strs = strarray.numel ();
  matches  = Array<octave_idx_type>(dim_vector(num_strs,1));
  nmatches = 0;
  for (i = 0; i < num_strs; i++)
  {
    if (octave::string::strncmpi (str, strarray(i), str.length ()) != 0)
    {
      matches(nmatches) = i;
      nmatches++;
    }
  }
  
  if (nmatches == 0)
  {
    non_match_str = strarray(0);
    for (i = 1; i < num_strs; i++)
    {
      non_match_str += ", " + strarray(i);
    }
    error (("validatestring: " + errstr + "does not match any of\n" + non_match_str).c_str ());
  }
  else if (nmatches == 1)
  {
    return octave_value (strarray(matches(0)));
  }
  else
  {
    min_len     = strarray(matches(0)).length ();
    min_len_idx = 0;
    for (i = 1; i < nmatches; i++)
    {
      len = strarray(matches(i)).length ();
      if (len < min_len)
      {
        min_len = len;
        min_len_idx = i;
      }
    }
    
    for (i = 0; i < nmatches; i++)
    {
      if (i != min_len_idx && ! octave::string::strncmpi (strarray(matches(min_len_idx)), strarray(matches(i)), min_len))
      {
        non_match_str = strarray(matches(0));
        for (i = 1; i < nmatches; i++)
        {
          non_match_str += ", " + strarray(matches(i));
        }
        error (("validatestring: " + errstr + "allows multiple unique matches:\n" + non_match_str).c_str ());
      }
    }
    return octave_value (strarray(matches(min_len_idx)));
  }
}

/*
%!shared strarray
%! strarray = {"octave" "Oct" "octopus" "octaves"};
%!assert (validatestring ("octave", strarray), "octave")
%!assert (validatestring ("oct", strarray), "Oct")
%!assert (validatestring ("octa", strarray), "octave")
%! strarray = {"abc1" "def" "abc2"};
%!assert (validatestring ("d", strarray), "def")

%!error <'xyz' does not match any> validatestring ("xyz", strarray)
%!error <DUMMY_TEST: 'xyz' does not> validatestring ("xyz", strarray, "DUMMY_TEST")
%!error <DUMMY_TEST: DUMMY_VAR does> validatestring ("xyz", strarray, "DUMMY_TEST", "DUMMY_VAR")
%!error <DUMMY_TEST: DUMMY_VAR \(argument #5\) does> validatestring ("xyz", strarray, "DUMMY_TEST", "DUMMY_VAR", 5)
%!error <'abc' allows multiple unique matches> validatestring ("abc", strarray)

## Test input validation
%!error validatestring ("xyz")
%!error validatestring ("xyz", {"xyz"}, "3", "4", 5, 6)
%!error <invalid number of character inputs> validatestring ("xyz", {"xyz"}, "3", "4", "5")
%!error <STR must be a character string> validatestring (1, {"xyz"}, "3", "4", 5)
%!error <STR must be a single row vector> validatestring ("xyz".', {"xyz"}, "3", "4", 5)
%!error <STRARRAY must be a cellstr> validatestring ("xyz", "xyz", "3", "4", 5)
%!error <FUNCNAME must be a single row vector> validatestring ("xyz", {"xyz"}, "33".', "4", 5)
%!error <VARNAME must be a single row vector> validatestring ("xyz", {"xyz"}, "3", "44".', 5)
%!error <POSITION must be> validatestring ("xyz", {"xyz"}, "3", "4", -5)
*/
