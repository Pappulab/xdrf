

int ftocstr(char *ds, int dl, char *ss, int sl)
{
    char *p;

    for (p = ss + sl; --p >= ss && *p == ' '; ) ;
    sl = p - ss + 1;
    dl--;
    ds[0] = 0;
    if (sl > dl)
        return 1;
    while (sl--)
	(*ds++ = *ss++);
    *ds = '\0';
    return 0;
}


int ctofstr(char *ds, int dl, char *ss)
{
    while (dl && *ss) {
	*ds++ = *ss++;
	dl--;
    }
    while (dl--)
	*ds++ = ' ';
    return 0;
}
