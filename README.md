Requirements:
=============
# To compile wxMailto, you need:
sudo apt-get install make g++ gnupg2 libgpgme11-dev libpoco-dev libgnutls-dev
# dependencies for code not yet merged
sudo apt-get install unixodbc libidn11-dev libmimetic-dev libgsasl7-dev

# For smart-card support:
sudo apt-get install pcscd pcsc-tools

# For USB CCID smart card readers:
sudo apt-get install libccid

# For compiling a debug build, you need
sudo apt-get install libpocoodbc9-dbg libpocomysql9-dbg libpocodata9-dbg libpocofoundation9-dbg

# You probably also want MySQL (unless you already have a database available for you)
sudo apt-get install mysql-server

#You need wxWidgets 3.0+. If it is not available for your system yet, here is how to download and build it:
<download>
<unpack>
cd <wxwidgets>
mkdir release
../configure --with-gtk --with-gnomeprint --enable-unicode --enable-threads --disable-debug --disable-compat24 --disable-compat26 --disable-compat28
make && sudo make install


#Create database
sudo mysqladmin -u root -p create wxMailto
sudo mysql -u root -p < ./create_database.sql

#Set up unixodbc (system) driver and (user) datasource
sudo odbcinst -i -l -d -f /usr/share/libmyodbc/odbcinst.ini
odbcinst -i -h -s -f ./datasource.template


#Compile appliction
make && sudo make install