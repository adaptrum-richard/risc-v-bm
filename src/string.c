void *memset(void *s, int c, unsigned long n)
{
	char *p = s;
	while(n--)
	{
		(*p++) = c;
	}
	return s;
}