# 数据库脚本，在mysql中运行
create database ServerDb;

USE ServerDb;
CREATE TABLE user(
    username char(50) NULL,
    pwd char(50) NULL
)ENGINE=InnoDB;

# 添加数据
INSERT INTO user(username, pwd) VALUES('song', 'aaa');

