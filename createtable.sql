CREATE TABLE baidu (
ID int primary key auto_increment,
url varchar(100),
name varchar(100),
description varchar(200));

CREATE UNIQUE INDEX baidu_id
ON baidu (ID);

CREATE INDEX baidu_name
ON baidu (name);

