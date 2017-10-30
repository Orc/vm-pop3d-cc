Name: vm-pop3d
Version: 1.1.3
Release: 1
Copyright: GPL
Group: Networking/Daemons
Source: ftp://sunsite.unc.edu/pub/Linux/system/mail/pop/%{name}-%{version}.tar.gz
Summary: vm-pop3d POP3 server
#Requires: pam >= 0.59
Prereq: /sbin/chkconfig
Obsoletes: gnu-pop3d
Vendor: ReedMedia
Packager: Radek Libovicky <libovicky@lirais.cz>
BuildRoot: /tmp/%{name}-%{version}-root

%description
vm-pop3d is a POP3 server.                                                                          
                                                                                                    
It supports alternative password files and mail spool directories;                                  
it can be used for setting up virtual email accounts -- mailboxes                                   
without real Unix owners for each. This will allow you to have                                      
multiple email accounts with the same name on one system.                                                                                                                                               
The POP3 protocol (by itself) is not secure.                                                                                                                                                            
vm-pop3d is based on Jakob Kaivo's gnu-pop3d (formerly called ids-pop3d).                           
I developed ten patches for gnu-pop3d version 0.9.8; accordingly,                                   
the first release of vm-pop3d is version 1.0.8. The code for vm-pop3d                               
version 1.0.8 should be identical to a virtual mail version 1.0-patched                             
gnu-pop3d 0.9.8.

%changelog
* Sat Mar 17 2001 Jeremy C. Reed
	vm-pop3d version 1.0.8 is gnu-pop3d 0.9.8 with virtual
	mail patch 1.0 integrated. In addition, the README and INSTALL
	documents are changed. All references to gnu-pop3d are changed
	(except for history or license). vm-pop3d 1.0.8 codewise is the
	same as a virtual mail 1.0-patched gnu-pop3d 0.9.8.

%prep
%setup
./configure --prefix=/usr

%build
make ROOT=$RPM_BUILD_ROOT
/usr/bin/strip --strip-all vm-pop3d

%clean
rm -rf $RPM_BUILD_ROOT

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d
make ROOT=$RPM_BUILD_ROOT install
mkdir -p $RPM_BUILD_ROOT/etc/pam.d
cp -f vm-pop3d.pamd $RPM_BUILD_ROOT/etc/pam.d/vm-pop3d
cp -f vm-pop3d.init $RPM_BUILD_ROOT/etc/rc.d/init.d/vm-pop3d


%pre
if [ -f /var/lock/subsys/vm-pop3d ]; then
   /etc/rc.d/init.d/vm-pop3d stop
fi

%post
/sbin/chkconfig --add vm-pop3d

%preun
if [ $1 = 0 ]; then
   if [ -f /var/lock/subsys/vm-pop3d ]; then
      /etc/rc.d/init.d/vm-pop3d stop
   fi
   /sbin/chkconfig --del vm-pop3d
fi

%files
%defattr(0644,root,root,0755)
%doc COPYING README* AUTHORS TODO INSTALL ./RFC/RFC*.txt
%attr(0755,root,root) /usr/sbin/vm-pop3d
%attr(0755,root,root) /etc/rc.d/init.d/vm-pop3d
%config /etc/pam.d/vm-pop3d

