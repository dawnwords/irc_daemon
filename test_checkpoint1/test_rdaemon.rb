#! /usr/bin/ruby
#
# 15-441 Project 2 Checkpoint 1 Script
#
#	This script tests for basic functionality of a standalone
#	routing daemon and server, aka there is no one else in the network.
#	This will allow you to test basic server->daemon responses before
#	worrying about adding more servers and daemons to your neck.  Each
#	test has an explanation of what should happen and why, so if you fail,
#	check the comments. The next script will test multiple routers and daemons.
#
# In checkpoint 1, we only test your server while using the given routing daemon
# binary. However, this script can also help you to test your standalone routing
# daemon when you are working on the routing part.
# 
#	To force you to properly read from the command line arguments, use proper
#	binary names, and read from configuration files, we are now starting
#	the routing daemon within the script.  Do *not* comment out the code to
#	start your daemon, and then start it manually.  You *must* be reading
#	proper arguments and configuration files.
#
#	If you're getting bind errors on andrew, change the ports that are written
#	to the config file within the write_config() function
#
# Enjoy!
#
# - 15-441 Staff

## SKELETON STOLEN FROM http://www.bigbold.com/snippets/posts/show/1785
require 'socket'
#$SAFE = 1

class RDAEMON

    def initialize(rdaemon, port, nick, channel)
        @rdaemon = rdaemon
        @port = port
        @nick = nick
        @channel = channel
    end

    def recv_data_from_rdaemon (timeout)
        pending_event = Time.now.to_i
        received_data = Array.new
        k = 0 
        flag = 0
        while flag == 0
            ## check for timeout
            time_elapsed = Time.now.to_i - pending_event
            if (time_elapsed > timeout)
                flag = 1
            end 
            ready = select([@rdaemon], nil, nil, 0.0001)
            next if !ready
            for s in ready[0]
                if s == @rdaemon then
                    next if @rdaemon.eof
                    s = @rdaemon.gets
                    received_data[k] = s
                    k= k + 1
                end
            end
        end
        return received_data
    end
    
		def send(s)
        # Send a message to the irc server and print it to the screen
        puts "--> #{s}"
        @rdaemon.send "#{s}\n", 0 
    end

    def connect()
        # Connect to the IRC rdaemon
        @rdaemon = TCPSocket.open(@rdaemon, @port)
    end
    
		def disconnect()
        @rdaemon.close
    end
    
		def ignore_reply
        data=recv_data_from_rdaemon(1)
    end

		def adduser(user)
			send("ADDUSER #{user}")
		end

		def removeuser(user)
			send("REMOVEUSER #{user}")
		end
		
		def addchan(chan)
			send("ADDCHAN #{chan}")
		end

		def removechan(chan)
			send("REMOVECHAN #{chan}")
		end
		
		def usertable()
			send("USERTABLE")
		end

		def chantable()
			send("CHANTABLE")
		end
		
		def checkok()
			data = recv_data_from_rdaemon(1)
			if(data[0] =~ /^OK*.\n/)
				return true
			else
				return false
			end
		end
		
		def checkok0()
			data = recv_data_from_rdaemon(1)
			if(data[0] =~ /^OK 0*.\n/)
				return true
			else
				return false
			end
		end

    def test_silence(timeout)
        data=recv_data_from_rdaemon(timeout)
        if (data.size > 0)
            return false
        else
            return true
        end
    end

end


##
# The main program.  Tests are listed below this point.  All tests
# should call the "result" function to report if they pass or fail.
##

def write_config()
afile = File.open("cp3nodes.conf", "w")
afile.puts "1 127.0.0.1 25500 25501 25502\n"
afile.puts "2 127.0.0.1 25503 25504 25505\n"
afile.puts "3 127.0.0.1 25506 25507 25508\n"
afile.close
end

# Go through the config file, looking for the "id" passed,
# and spawn it
def spawn_daemon(id)
afile = File.open("cp3nodes.conf", "r")
afile.each_line do |line|
	line.chomp!
	(nodeid, ip, lport, dport, sport)=line.split
	if(nodeid.to_i == id)  # dont forget to convert to int
		system("./srouted -i #{id} -c cp3nodes.conf &")
		return 1 
	end
end
end

def conn_daemon(id)
afile = File.open("cp3nodes.conf", "r")
afile.each_line do |line|
	line.chomp!
	(nodeid, ip, lport, dport, sport)=line.split
	if(nodeid.to_i == id)  # dont forget to convert to int
		rdaemon = RDAEMON.new(ip, dport.to_i, '', '')
		rdaemon.connect()
		return rdaemon 
	end
end
end


$total_points = 0

def test_name(n)
    puts "////// #{n} \\\\\\\\\\\\"
    return n
end

def result(n, passed_exp, failed_exp, passed, points)
    explanation = nil
    if (passed)
	print "(+) #{n} passed\n"
	$total_points += points
	explanation = passed_exp
    else
	print "(-) #{n} failed\n"
	explanation = failed_exp
    end

    if (explanation)
	puts ": #{explanation}"
    else
	puts ""
    end
end

def eval_test(n, passed_exp, failed_exp, passed, points = 1)
    result(n, passed_exp, failed_exp, passed, points)
    exit(0) if !passed
end

#### WRITE CONFIG FILE
#
# YOU MAY EDIT THE PORT NUMBERS HERE ONLY
# (to avoid port collisions while testing on andrew)
write_config()

begin

spawn_daemon(1)							# spawn node ID 1, which translates to ./srouted -i 1 -c cp3nodes.conf
sleep(2)
rdaemon = conn_daemon(1)		# connect to node ID 1

################## ADDUSER TEST #################
# When your server sends an "ADDUSER nick" to the
# routing daemon, it will respond with an OK if
# successful, and not respond on failure
	tn = test_name("ADDUSER")
	rdaemon.adduser("gnychis")
	puts "<-- Waiting for OK"
	eval_test(tn, nil, nil, rdaemon.checkok())

################## REMOVEUSER TEST #################
# When your server sends a "REMOVEUSER nick" to the
# routing daemon, it should respond with "OK" if the
# user was in it's local users list, if not then
# it responds with nothing on failure.
	tn = test_name("REMOVEUSER")
	rdaemon.removeuser("gnychis")
	puts "<-- Waiting for OK"
	eval_test(tn, nil, nil, rdaemon.checkok())

################## ADDCHAN TEST #################
# When your server sends an "ADDCHAN nick" to the
# routing daemon, it will respond with an OK if
# successful, and not respond on failure
	tn = test_name("ADDCHAN")
	rdaemon.addchan("#linux")
	puts "<-- Waiting for OK"
	eval_test(tn, nil, nil, rdaemon.checkok())

################## REMOVECHAN TEST #################
# When your server sends a "REMOVECHAN nick" to the
# routing daemon, it should respond with "OK" if the
# chan was in it's local chans list, if not then
# it responds with nothing on failure.
	tn = test_name("REMOVECHAN")
	rdaemon.removechan("#linux")
	puts "<-- Waiting for OK"
	eval_test(tn, nil, nil, rdaemon.checkok())

################# SILENCE TESTS ####################
# Your routing daemon should not crash and should
# fail silently commands which are improper.  Such
# as trying to "ADDUSER gnychis" twice.  The first
# add should return "OK", the second add should
# return no response.
puts "If you have implemented a non-blocking routing
daemon and would like to test it, uncomment out the
SILENCE ADDUSER tests\n\n"
#	tn = test_name("SILENCE ADDUSER")
	
	# Test for ADDUSER silence first
#	rdaemon.adduser("dave")
#	rdaemon.ignore_reply()
#	rdaemon.adduser("dave")
#	eval_test(tn, nil, nil, rdaemon.test_silence(1))

	# Test for silence on double ADDCHANNEL of the same
	#  channel now
#	tn = test_name("SILENCE ADDCHANNEL")
#	rdaemon.addchan("#ruby")
#	rdaemon.ignore_reply()
#	rdaemon.addchan("#ruby")
#	eval_test(tn, nil, nil, rdaemon.test_silence(1))

################## USERTABLE TEST #################
# When the server asks for the user table, the 
# routing daemon responds with:
# 	OK size
# 	nick nexthop distance
# 	.....................
#
# Your local users are NOT in the user table,
# so when the script asks, it should return "OK 0"
# after we re-add "gnychis" and ask for the table list
	tn = test_name("USERTABLE")
	rdaemon.adduser("gnychis")
	rdaemon.ignore_reply()
	rdaemon.usertable()
	puts "<-- Waiting for OK 0"
	eval_test(tn, nil, nil, rdaemon.checkok0())

################## CHANTABLE TEST #################
# When the server asks for the chan table, the 
# routing daemon responds with:
# 	OK size
# 	nick nexthop distance
# 	.....................
#
# Your local chans are NOT in the chan table,
# so when the script asks, it should return "OK 0"
# after we re-add "gnychis" and ask for the table list
	tn = test_name("CHANTABLE")
	rdaemon.addchan("#linux")
	rdaemon.ignore_reply()
	rdaemon.chantable()
	puts "<-- Waiting for OK 0"
	eval_test(tn, nil, nil, rdaemon.checkok0())


rescue Interrupt
rescue Exception => detail
    puts detail.message()
    print detail.backtrace.join("\n")
ensure
    rdaemon.disconnect()
    puts "Your cool points earned: #{$total_points}"
    puts ""
		puts "Stay tuned for a checkpoint 4 which will help you test multiple daemons"
		system("killall srouted")
end
