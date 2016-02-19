#ifndef _CRUD_H_
#define _CRUD_H_
#define HYPERLINK "<a href='http://%s'>%s</a><br>%s"
int sql_connect(const char *User, const char *Password, const char *DBName);
int sql_disconnect();
int query_result(const char *name, char **buf);

#endif 
