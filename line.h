struct line {
    char *x;  /* contents */
    size_t l; /* length */
    size_t a; /* allocated */
};

void line_read(FILE *, struct line *);
_Bool line_write(FILE *out, struct line *l);

void line_cat(struct line *, struct line *);
void line_copy(struct line *, struct line *);

_Bool line_blank(struct line *);
_Bool line_empty(struct line *);
_Bool line_hdr_cont(struct line *);
_Bool line_starts(struct line *, const char *);
