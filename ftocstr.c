
/*____________________________________________________________________________
 |
 | ftocstr.c - Fortran-to-C and C-to-Fortran string conversion utilities.
 |
 | Fortran strings are fixed-length, space-padded, and not null-terminated.
 | C strings are null-terminated and variable-length. These two routines
 | convert between the two representations so the Fortran wrapper functions
 | in libxdrf can pass strings to the underlying C XDR routines.
 |
*/

/*
 * ftocstr - Convert a Fortran string to a C string.
 *
 * Copies the Fortran string 'ss' (of length 'sl') into the C buffer 'ds'
 * (of max length 'dl'), stripping trailing spaces and adding a null
 * terminator.
 *
 * Parameters:
 *   ds - Destination C string buffer.
 *   dl - Maximum length of destination buffer (including null terminator).
 *   ss - Source Fortran string (not null-terminated, may be space-padded).
 *   sl - Length of source Fortran string.
 *
 * Returns: 0 on success, 1 if the trimmed source string is too long for
 *          the destination buffer.
 */
int ftocstr(char *ds, int dl, char *ss, int sl)
{
    char *p;

    /* Scan backwards from end of Fortran string to skip trailing spaces */
    for (p = ss + sl; --p >= ss && *p == ' '; ) ;
    /* Compute the actual content length (without trailing spaces) */
    sl = p - ss + 1;
    dl--;           /* Reserve one byte for the null terminator */
    ds[0] = 0;
    if (sl > dl)    /* Source content won't fit in destination */
        return 1;
    /* Copy characters one by one */
    while (sl--)
	(*ds++ = *ss++);
    *ds = '\0';     /* Null-terminate the C string */
    return 0;
}


/*
 * ctofstr - Convert a C string to a Fortran string.
 *
 * Copies the null-terminated C string 'ss' into the fixed-length Fortran
 * buffer 'ds' (of length 'dl'), padding the remainder with spaces.
 *
 * Parameters:
 *   ds - Destination Fortran string buffer (fixed length, no null terminator).
 *   dl - Length of destination buffer.
 *   ss - Source C string (null-terminated).
 *
 * Returns: 0 always.
 */
int ctofstr(char *ds, int dl, char *ss)
{
    /* Copy C string characters into Fortran buffer */
    while (dl && *ss) {
	*ds++ = *ss++;
	dl--;
    }
    /* Pad remaining space with blanks (Fortran convention) */
    while (dl--)
	*ds++ = ' ';
    return 0;
}
