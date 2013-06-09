#! /usr/bin/env ruby
#
# 15-441 Project2 Script2
#
#	We have created the following network for you:
#
#   2
#  / \
# 1   4
#  \ /
#   3
#
#	The following nodes have the following channels and users:
#
#	1 - gnychis, #linux
#	2 - #ruby
#	3 - #ruby, #networks
#	4 - dga
#
#
# Goodluck!

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
    rescue => err
	puts "Could not connect to IRC daemon on port #{@port}"
	throw err
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

		def usertablet1()
			send("USERTABLE")
			data=recv_data_from_rdaemon(1)
			if(data[0] =~ /^OK 2*.\n/)
				puts "--> correct size of user table, 2\n"
			else
				return false				
			end

			responsehash = Hash.new

			index=1

			while(index < data.size)
				(name, nexthop, distance) = data[index].split
				responsehash[name] = Array.new
				responsehash[name][0] = nexthop
				responsehash[name][1] = distance
				index += 1
			end

			####### check for gnychis in response ##################
			if(responsehash.has_key?("gnychis"))
				puts "--> correct, response includes gnychis\n"
				# check the next hop value
				if(responsehash["gnychis"][0].to_i == 1)
					puts "	* correct, next hop to gnychis from 2 is 1\n"
				else
					puts "	! incorrect, next hop to gnychis from 2 should be 1, your value was: #{responsehash["gnychis"][0]}\n"
					return false
				end
				# check the distance value
				if(responsehash["gnychis"][1].to_i == 1)
					puts "	* correct, distance to gnychis from 2 is 1\n"
				else
					puts "	! incorrect, distance to gnychis from 2 should be 1, your value was: #{responsehash["gnychis"][1]}\n"
					return false
				end
			else
				return false
			end

			####### check for dga in response ###############
			if(responsehash.has_key?("dga"))
				puts "--> correct, response includes dga\n"
				# check the next hop value
				if(responsehash["dga"][0].to_i == 4)
					puts "	* correct, next hop to dga from 2 is 4\n"
				else
					puts "	! incorrect, next hop to dga from 2 should be 4, your value was: #{responsehash["gnychis"][0]}\n"
					return false
				end
				# check the distance value
				if(responsehash["dga"][1].to_i == 1)
					puts "	* correct, distance to dga from 2 is 1\n"
				else
					puts "	! incorrect, distance to dga from 2 should be 1, your value was: #{responsehash["gnychis"][1]}\n"
					return false
				end
			else
				return false
			end

			return true  # You must have survived!
		end

		def nexthop(user, nexthop, distance)
			send("NEXTHOP #{user}")

			data=recv_data_from_rdaemon(1)
		
			(ok, rnexthop, rdistance)=data[0].split

			if(ok != "OK")
				puts "--> improper format:  OK <nexthop> <distance>\n"
				return false
			end

			if(rnexthop.to_i != nexthop)
				puts "--> next hop to #{user} should be #{nexthop}.  Your value: #{rnexthop}\n"
				return false
			end

			if(rdistance.to_i != distance)
				puts "--> distance to #{user} should be #{distance}.  Your value: #{rdistance}\n"
				return false
			end

			puts "--> correct nexthop of #{nexthop} and distance of #{distance}\n"
			return true

		end

		def nexthopnone(user)

			send("NEXTHOP #{user}")

			data=recv_data_from_rdaemon(1)

			if(data[0] =~ /^NONE*.\n/)
				puts "--> correct NONE response for non-existant user\n"
				return true
			else
				puts "--> your server should return NONE if you ask for a NEXTHOP on a non-existant user\n"
				return false
			end

		end
		
		def nexthops(chan, source, nexthops)
			send("NEXTHOPS #{source} #{chan}")
			snexthops=nexthops # save "good" value in string format for easy printing

			# The nexthops can be returned in any order, so lets toss them in a hash
			nexthopshash = Hash.new
			nexthops.split(' ').each { |i| nexthopshash[i.to_i] = nil }

			data=recv_data_from_rdaemon(1)

			if(snexthops =~ /NONE/ && data[0] !~ /NONE/)
				puts "--> should be NONE\n"
				return false
			elsif(snexthops =~ /NONE/)
				puts "--> NONE is correct!\n"
				return true
			end

		
			if(!data[0] =~ /^OK/)
				puts "--> improper format:  OK <source> <next-hop> ...\n"
				return false
			end

			# lets get rid of the "OK" and use the same functionality to create a new hash of return vals
			data[0] = data[0].delete "OK"
			srnexthops=data[0]	# save old value in string format for easy printing
			rnexthopshash = Hash.new
			data[0].split(' ').each { |i| rnexthopshash[i.to_i] = nil }

			if(nexthopshash.keys.sort != rnexthopshash.keys.sort)
				puts "--> Invalid next hops: #{srnexthops}  Should have been:  #{snexthops}\n"
				puts "    *Note: order does not matter\n"
				return false
			else
				puts "--> Next hops were correct!\n\n"
			end

			return true

		end

		def printdata()
			data=recv_data_from_rdaemon(1)
			data.each { |x| puts x}
		end

end


##
# The main program.  Tests are listed below this point.  All tests
# should call the "result" function to report if they pass or fail.
##

def write_config()
    File.open("cp4node1.conf", "w") { |afile|
	afile.puts "1 127.0.0.1 25500 25501 25502"
	afile.puts "2 127.0.0.1 25503 25504 25505"
	afile.puts "3 127.0.0.1 25506 25507 25508"
    }

    File.open("cp4node2.conf", "w") { |afile|
	afile.puts "1 127.0.0.1 25500 25501 25502"
	afile.puts "2 127.0.0.1 25503 25504 25505"
	afile.puts "4 127.0.0.1 25509 25510 25511"
    }

    File.open("cp4node3.conf", "w") { |afile|
	afile.puts "1 127.0.0.1 25500 25501 25502"
	afile.puts "3 127.0.0.1 25506 25507 25508"
	afile.puts "4 127.0.0.1 25509 25510 25511"
    }

    File.open("cp4node4.conf", "w") { |afile|
	afile.puts "4 127.0.0.1 25509 25510 25511"
	afile.puts "2 127.0.0.1 25503 25504 25505"
	afile.puts "3 127.0.0.1 25506 25507 25508"
    }
end

# Go through the config file, looking for the "id" passed,
# and spawn it
def spawn_daemon(file,id)
afile = File.open(file, "r")
afile.each_line do |line|
	line.chomp!
	(nodeid, ip, lport, dport, sport)=line.split
	if(nodeid.to_i == id)  # dont forget to convert to int
		puts "./srouted -i #{id} -c #{file} -a 1 &"
	        system("./srouted -i #{id} -c #{file} -a 1 &")
		return 1 
	end
end
end

def conn_daemon(file,id)
afile = File.open(file, "r")
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
	puts "(+) #{n} passed"
	$total_points += points
	explanation = passed_exp
    else
	puts "(-) #{n} failed"
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

# Spawn the daemons.  This runs command lines of
# ./srouted -i ## -c cprnode##.conf  (where ## == 1 through 4)
spawn_daemon("cp4node1.conf", 1)
spawn_daemon("cp4node2.conf", 2)
spawn_daemon("cp4node3.conf", 3)
spawn_daemon("cp4node4.conf", 4)

puts "Sleeping for 2 seconds to let the IRC servers finish starting..."
sleep(2)

puts "Connecting to the daemons..."
# Now connect a TCP socket to the daemons
rdaemon1 = conn_daemon("cp4node1.conf", 1)
rdaemon2 = conn_daemon("cp4node2.conf", 2)
rdaemon3 = conn_daemon("cp4node3.conf", 3)
rdaemon4 = conn_daemon("cp4node4.conf", 4)

puts "Sleeping for 5 seconds to let everything settle."
sleep(5)

################## USERTABLE TEST #################
# I am going to add a user on node1 and node4,
# then test the USERTABLE response at 2 and 3
	tn = test_name("USERTABLE @ Node 2")
	rdaemon1.adduser("gnychis")
	rdaemon1.ignore_reply()
	rdaemon4.adduser("dga")
	rdaemon4.ignore_reply()
	eval_test(tn, nil, nil, rdaemon2.usertablet1())
	
	tn = test_name("USERTABLE @ Node 3")
	eval_test(tn, nil, nil, rdaemon3.usertablet1())

################## NEXTHOP TEST #################
# Test the return value of NEXTHOP in several
# nodes
	tn = test_name("NEXTHOP gnychis @ Node 4")
	eval_test(tn, nil, nil, rdaemon4.nexthop("gnychis", 2, 2))
	
	tn = test_name("NEXTHOP dga @ Node 1")
	eval_test(tn, nil, nil, rdaemon1.nexthop("dga", 2, 2))

################## NONE TEST #####################
# Your routing daemon should respond with "NONE"
# to all of these tests
	tn = test_name("NONE from invalid NEXTHOP")
	# a server should not have a hop response to a local user
	eval_test(tn, nil, nil, rdaemon1.nexthopnone("gnychis"))
	# a server responds with none to a non-existant user
	eval_test(tn, nil, nil, rdaemon2.nexthopnone("srini"))
	# NEXTHOPS is used for channels...
	eval_test(tn, nil, nil, rdaemon3.nexthopnone("#linux"))
	# dga should not be in this response, since he is a local user
	eval_test(tn, nil, nil, rdaemon4.nexthopnone("dga"))

################# NEXTHOPS #######################
# NEXTHOPS should determine where you need to
# forward a message from a given source recipients
# 
# To conduct these tests, we add #linux to node 1,
# #ruby to node 2 and 3, and #networks to node 3
#

	rdaemon1.addchan("#linux")
	rdaemon1.ignore_reply()
        rdaemon2.addchan("#ruby")
        rdaemon2.ignore_reply()
	rdaemon3.addchan("#ruby")
        rdaemon3.ignore_reply()
        rdaemon3.addchan("#networks")
	rdaemon3.ignore_reply()
	
	

	# If a user on node 4 sends a message to #linux it should
	# go through node 2
	tn = test_name("NEXTHOPS #linux from node 4")
	rdaemon4.nexthops("#linux", 4, "2")

	# 4 should have to forward a message to #ruby to
	# both nodes 2 and 3
	tn = test_name("NEXTHOPS #ruby from node 4")
	rdaemon4.nexthops("#ruby", 4, "2 3")

	tn = test_name("NEXTHOPS #ruby from node 2")
	rdaemon2.nexthops("#ruby", 2, "1")

	tn = test_name("NEXTHOPS #ruby from node 1")
	rdaemon1.nexthops("#ruby", 1, "3 2")

	tn = test_name("NEXTHOPS #linux from node 1")
	rdaemon1.nexthops("#linux", 1, "NONE")

################## CHANTABLE ########################
# This can be a little difficult to produce properly,
# but it should include hops to all channels given
# all possible sources. It's to help us grade.
	
	tn = test_name("CHANTABLE check")
	rdaemon2.send("CHANTABLE")

puts "Okay, for me to code checking of CHANTABLE, 
given that there can be multiple responses per
channel and any order of hops, to get this checkpoint
out earlier, hand check it yourself, here are the
responses you should get, no more, no less, but
order of hops doesn't matter:\n\n"

puts "OK 4
#linux 1 
#ruby 2 1
#ruby 3 
#networks 3\n"


puts "\nYour Response:\n\n"
			rdaemon2.printdata();


rescue Interrupt
rescue Exception => detail
    puts detail.message()
    print detail.backtrace.join("\n")
ensure
#    rdaemon.disconnect()
		puts "\n\nGoodluck with your final submission, don't forget to submit it properly!\n"
		system("killall srouted")
end
