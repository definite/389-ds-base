#{{PERL-EXEC}}
#
# BEGIN COPYRIGHT BLOCK
# Copyright (C) 2001 Sun Microsystems, Inc. Used by permission.
# Copyright (C) 2005 Red Hat, Inc.
# All rights reserved.
# END COPYRIGHT BLOCK
#

sub usage {
	print(STDERR "Usage: $0 [-v] -D rootdn { -w password | -w - | -j filename } \n");
	print(STDERR "        -n instance | {-s include}* [{-x exclude}*] [-O] [-c]\n");
	print(STDERR "        [-g [string]] [-G namespace_id] {-i filename}*\n");
	print(STDERR " Opts: -D rootdn     - Directory Manager\n");
	print(STDERR "     : -w password   - Directory Manager's password\n");
	print(STDERR "     : -w -          - Prompt for Directory Manager's password\n");
	print(STDERR "     : -j filename   - Read Directory Manager's password from file\n");
	print(STDERR "     : -n instance   - instance to be imported to\n");
	print(STDERR "     : -i filename   - input ldif file(s)\n");
	print(STDERR "     : -s include    - included suffix\n");
	print(STDERR "     : -x exclude    - excluded suffix(es)\n");
	print(STDERR "     : -O            - only create core db, no attr indexes\n");
	print(STDERR "     : -c size       - merge chunk size\n");
	print(STDERR "     : -g [string]   - string is \"none\" or \"deterministic\"\n");
	print(STDERR "     :          none - unique id is not generated\n");
	print(STDERR "     : deterministic - generate name based unique id (-G name)\n");
	print(STDERR "     :    by default - generate time based unique id\n");
	print(STDERR "     : -G name       - namespace id for name based uniqueid (-g deterministic)\n");
	print(STDERR "     : -E            - Encrypt data when importing\n");
	print(STDERR "     : -v            - verbose\n");
}

@ldiffiles = (
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	""
);
@included = (
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	""
);
@excluded = (
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	""
);
$maxidx = 50;
$instance = "";
$noattrindexes = 0;
$mergechunksiz = 0;
$genuniqid = "time";
$uniqidname = "";
$taskname = "";
$dsroot = "{{DS-ROOT}}";
$mydsroot = "{{MY-DS-ROOT}}";
$verbose = 0;
$rootdn = "";
$passwd = "";
$passwdfile = "";
$i = 0;
$ldifi = 0;
$incli = 0;
$excli = 0;
$encrypt_on_import = 0;
while ($i <= $#ARGV) {
	if ( "$ARGV[$i]" eq "-i" ) {	# ldiffiles
		$i++;
		if ($ldifi < $maxidx) {
			$ldiffiles[$ldifi] = $ARGV[$i]; $ldifi++;
		} else {
			&usage; exit(1);
		}
	} elsif ("$ARGV[$i]" eq "-s") {	# included suffix
		$i++;
		if ($incli < $maxidx) {
			$included[$incli] = $ARGV[$i]; $incli++;
		} else {
			&usage; exit(1);
		}
	} elsif ("$ARGV[$i]" eq "-x") {	# excluded suffix
		$i++;
		if ($excli < $maxidx) {
			$excluded[$excli] = $ARGV[$i]; $excli++;
		} else {
			&usage; exit(1);
		}
	} elsif ("$ARGV[$i]" eq "-n") {	# instance
		$i++; $instance = $ARGV[$i];
	} elsif ("$ARGV[$i]" eq "-D") {	# Directory Manager
		$i++; $rootdn = $ARGV[$i];
	} elsif ("$ARGV[$i]" eq "-w") {	# Directory Manager's password
		$i++; $passwd = $ARGV[$i];
	} elsif ("$ARGV[$i]" eq "-j") { # Read Directory Manager's password from a file
		$i++; $passwdfile = $ARGV[$i];
	} elsif ("$ARGV[$i]" eq "-O") {	# no attr indexes
		$noattrindexes = 1;
	} elsif ("$ARGV[$i]" eq "-c") {	# merge chunk size
		$i++; $mergechunksiz = $ARGV[$i];
	} elsif ("$ARGV[$i]" eq "-g") {	# generate uniqueid
		if (("$ARGV[$i+1]" ne "") && !("$ARGV[$i+1]" =~ /^-/)) {
			$i++;
			if ("$ARGV[$i]" eq "none") {
				$genuniqid = $ARGV[$i];
			} elsif ("$ARGV[$i]" eq "deterministic") {
				$genuniqid = $ARGV[$i];
			}
		}
	} elsif ("$ARGV[$i]" eq "-G") {	# namespace id
		$i++; $uniqidname = $ARGV[$i];
	} elsif ("$ARGV[$i]" eq "-v") {	# verbose
		$verbose = 1;
	} elsif ("$ARGV[$i]" eq "-E") {	# encrypt on import
		$encrypt_on_import = 1;
	} else {
		&usage; exit(1);
	}
	$i++;
}
if ($passwdfile ne ""){
# Open file and get the password
	unless (open (RPASS, $passwdfile)) {
		die "Error, cannot open password file $passwdfile\n";
	}
	$passwd = <RPASS>;
	chomp($passwd);
	close(RPASS);
} elsif ($passwd eq "-"){
# Read the password from terminal
	die "The '-w -' option requires an extension library (Term::ReadKey) which is not\n",
	    "part of the standard perl distribution. If you want to use it, you must\n",
	    "download and install the module. You can find it at\n",
	    "http://www.perl.com/CPAN/CPAN.html\n";
# Remove the previous line and uncomment the following 6 lines once you have installed Term::ReadKey module.
# use Term::ReadKey; 
#	print "Bind Password: ";
#	ReadMode('noecho'); 
#	$passwd = ReadLine(0); 
#	chomp($passwd);
#	ReadMode('normal');
}
if (($instance eq "" && $included[0] eq "") || $ldiffiles[0] eq "" || $rootdn eq "" || $passwd eq "") { &usage; exit(1); }
($s, $m, $h, $dy, $mn, $yr, $wdy, $ydy, $r) = localtime(time);
$mn++; $yr += 1900;
$taskname = "import_${yr}_${mn}_${dy}_${h}_${m}_${s}";
$dn = "dn: cn=$taskname, cn=import, cn=tasks, cn=config\n";
$misc = "changetype: add\nobjectclass: top\nobjectclass: extensibleObject\n";
$cn =  "cn: $taskname\n";
if ($instance ne "") {
	$nsinstance = "nsInstance: ${instance}\n";
}
$i = 0;
$nsldiffiles = "";
while ("" ne "$ldiffiles[$i]") {
	$nsldiffiles = "${nsldiffiles}nsFilename: $ldiffiles[$i]\n";
	$i++;
}
$i = 0;
$nsincluded = "";
while ("" ne "$included[$i]") {
	$nsincluded = "${nsincluded}nsIncludeSuffix: $included[$i]\n";
	$i++;
}
$i = 0;
$nsexcluded = "";
while ("" ne "$excluded[$i]") {
	$nsexcluded = "${nsexcluded}nsExcludeSuffix: $excluded[$i]\n";
	$i++;
}
$nsnoattrindexes = "";
if ($noattrindexes != 0) { $nsnoattrindexes = "nsImportIndexAttrs: false\n"; }
$nsimportencrypt = "";
if ($encrypt_on_import != 0) { $nsimportencrypt = "nsImportEncrypt: true\n"; }
$nsmergechunksiz = "nsImportChunkSize: ${mergechunksiz}\n"; 
$nsgenuniqid = "nsUniqueIdGenerator: ${genuniqid}\n"; 
$nsuniqidname = "";
if ($uniqidname ne "") { $nsuniqidname = "nsUniqueIdGeneratorNamespace: ${uniqidname}\n"; }
$entry = "${dn}${misc}${cn}${nsinstance}${nsincluded}${nsexcluded}${nsldiffiles}${nsnoattrindexes}${nsimportencrypt}${nsmergechunksiz}${nsgenuniqid}${nsuniqidname}";
$vstr = "";
if ($verbose != 0) { $vstr = "-v"; }
chdir("$dsroot{{SEP}}shared{{SEP}}bin");
open(FOO, "| $dsroot{{SEP}}shared{{SEP}}bin{{SEP}}ldapmodify $vstr -h {{SERVER-NAME}} -p {{SERVER-PORT}} -D \"$rootdn\" -w \"$passwd\" -a" );
print(FOO "$entry");
close(FOO);
