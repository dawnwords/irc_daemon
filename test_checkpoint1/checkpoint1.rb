#! /usr/bin/env ruby
#
# 15-441 Project2 Checkpoint 1 Script (Last updated: Feb 24 3:00 PM)
#
# Xin Zhang <xzhang1@cs.cmu.edu>
# 
#
# In this checkpoint we mainly test basic interfaces
# between a STANDALONE server and routing daemon. We provide you 
# with a working routing daemon binary so that you only need to take
# care of the server part. Each test is described below.
#
# Enjoy!
#

## SKELETON STOLEN FROM http://www.bigbold.com/snippets/posts/show/1785
require 'socket'
require 'rdaemon'
require 'irc'
#$SAFE = 1

$SERVER = "127.0.0.1"
$PORT = 14000  ########## DONT FORGET TO CHANGE THIS


##
# The main program.  Tests are listed below this point.  All tests
# should call the "result" function to report if they pass or fail.
##

$total_points = 0

def test_name(n)
    puts "////// #{n} \\\\\\\\\\\\"
    return n
end

def result(n, passed_exp, failed_exp, passed, points)
    explanation = nil
    if (passed)
	print "(+) #{n} passed"
	$total_points += points
	explanation = passed_exp
    else
	print "(-) #{n} failed"
	explanation = failed_exp
    end

    if (explanation)
	puts ": #{explanation}"
    else
	puts ""
    end
end

def eval_test(n, passed_exp, failed_exp, passed, points = 2)
    result(n, passed_exp, failed_exp, passed, points)
#   exit(0) if !passed
end

def write_config()
#afile = File.open("irc_protocol.conf", "w")
#afile.puts "1 127.0.0.1 34105 34106 34107\n"
#afile.close
end

# Go through the config file, looking for the "id" passed,
# and spawn it
def spawn_daemon(config,id)
afile = File.open(config, "r")
afile.each_line do |line|
	line.chomp!
	(nodeid, ip, lport, dport, sport)=line.split
	if(nodeid.to_i == id)  # dont forget to convert to int
		system("./srouted -i #{id} -c #{config} &> /dev/null &")
		return 1 
	end
end
end

# Go through the config file, looking for the "id" passed,
# and spawn it
def spawn_server(config,id)
afile = File.open(config, "r")
afile.each_line do |line|
	line.chomp!
	(nodeid, ip, lport, dport, sport)=line.split
	if(nodeid.to_i == id)  # dont forget to convert to int
		system("./sircd #{id} #{config} &> /dev/null &")
		return 1 
	end
end
end

def conn_server(config,id)
afile = File.open(config, "r")
afile.each_line do |line|
	line.chomp!
	(nodeid, ip, lport, dport, sport)=line.split
	if(nodeid.to_i == id)  # dont forget to convert to int
		server = IRC.new(ip, sport.to_i, '', '')
		server.connect()
		puts "server connects"
		return server
	end
end
end

def conn_daemon(config,id)
afile = File.open(config, "r")
afile.each_line do |line|
	line.chomp!
	(nodeid, ip, lport, dport, sport)=line.split
	if(nodeid.to_i == id)  # dont forget to convert to int
		rdaemon = RDAEMON.new(ip, dport.to_i, '', '')
		rdaemon.connect()
		puts "routing daemon connects"
		return rdaemon 
	end
end
end

write_config()
spawn_daemon("irc_protocol.conf", 1)
spawn_server("irc_protocol.conf", 1)
sleep(1)
irc = conn_server("irc_protocol.conf", 1)
rdaemon = conn_daemon("irc_protocol.conf", 1)

puts "Letting everything settle..."
sleep(2)

begin

################## ADDUSER TEST #################
# When your server sends an "ADDUSER nick" to the
# routing daemon, it will respond with an OK if
# successful, and not respond on failure
	tn = test_name("ADDUSER")
	irc.send_nick("gnychis")
	irc.send_user("please give me :The MOTD")
	puts "<-- Waiting for OK"
	sleep(2)
	eval_test(tn, nil, nil, rdaemon.checkok())


############## ADDCHAN TEST ###################
# When your server sends an "ADDCHAN nick" to the
# routing daemon, it will respond with an OK if
# successful, and not respond on failure
	tn = test_name("ADDCHAN")
	#rdaemon.addchan("#linux")
	irc.raw_join_channel("gnychis", "#linux")
        irc.ignore_reply()
	puts "<-- Waiting for OK"
	sleep(2)
	eval_test(tn, nil, nil, rdaemon.checkok())


################## REMOVECHAN TEST #################
# When your server sends a "REMOVECHAN nick" to the
# routing daemon, it should respond with "OK" if the
# channel was in it's local users list, if not then
# it responds with nothing on failure.
	tn = test_name("REMOVECHAN")
	irc.part_channel("gnychis", "#linux")
	puts "<-- Waiting for OK"
	sleep(2)
	eval_test(tn, nil, nil, rdaemon.checkok())


################## REMOVEUSER TEST #################
# When your server sends a "REMOVEUSER nick" to the
# routing daemon, it should respond with "OK" if the
# user was in it's local users list, if not then
# it responds with nothing on failure.
	tn = test_name("REMOVEUSER")
	irc.quit_user("gnychis")
	puts "<-- Waiting for OK"
	sleep(2)
	eval_test(tn, nil, nil, rdaemon.checkok())
  

rescue Interrupt
rescue Exception => detail
    puts detail.message()
    print detail.backtrace.join("\n")
ensure
		system("killall sircd")
		system("killall srouted")
    irc.disconnect()
    puts "Your score: #{$total_points} out of 8"
    puts ""
    puts "Good luck with the rest of the project!"
    puts "Remember to commit code into tags/checkpoint1"
end
