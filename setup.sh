#!/bin/sh

#Dependencies: gnutls, GPGME, Libidn, iOdbc, mimetic
sudo apt-get install g++ mysql-server unixodbc libmyodbc libgnutls-dev libgpgme11-dev libidn11-dev unixodbc-dev libmimetic-dev libgsasl7-dev libpoco-dev libgcrypt20-dev

#Create database
sudo mysqladmin -u root -p create wxMailto
sudo mysql -u root -p < ./create_database.sql

#Set up unixodbc (system) driver and (user) datasource
sudo odbcinst -i -l -d -f /usr/share/libmyodbc/odbcinst.ini
odbcinst -i -h -s -f ./datasource.template

make
