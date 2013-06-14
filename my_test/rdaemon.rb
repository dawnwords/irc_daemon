#! /usr/bin/ruby

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
										puts s
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

		def createuserhash(data)
			responsehash = Hash.new

			index=1

			while(index < data.size)
				(name, nexthop, distance) = data[index].split
				responsehash[name] = Array.new
				responsehash[name][0] = nexthop
				responsehash[name][1] = distance
				index += 1
			end

			return responsehash
		end

		def nexthop(user, nexthop, distance)
			rval=true
			send("NEXTHOP #{user}")

			data=recv_data_from_rdaemon(1)
			
			if(data.size==0)
				puts "--> Nil response??"
				return false
			end

			(ok, rnexthop, rdistance)=data[0].split

			if(ok != "OK")
				puts "--> improper format:  OK <nexthop> <distance> ... #{data[0]}\n"
				rval=false
			end

			if(rnexthop.to_i != nexthop)
				puts "--> next hop to #{user} should be #{nexthop}.  Your value: #{rnexthop}\n"
				rval=false
			end

			if(rdistance.to_i != distance)
				puts "--> distance to #{user} should be #{distance}.  Your value: #{rdistance}\n"
				rval=false
			end

			if(rval==true)
				puts "--> correct nexthop of #{nexthop} and distance of #{distance}\n"
			end

			return rval

		end

		def checkuserhash(responsehash, user, nexthop, distance)
			####### check for dga in response ###############
			if(responsehash.has_key?(user))
				puts "--> correct, response includes #{user}\n"
				# check the next hop value
				if(responsehash[user][0].to_i == nexthop)
					puts "	* correct, next hop to #{user} is #{nexthop}\n"
				else
					puts "	! incorrect, next hop to #{user} is #{nexthop}, your value was: #{responsehash[user][0]}\n"
					return false
				end
				# check the distance value
				if(responsehash[user][1].to_i == distance)
					puts "	* correct, distance to #{user} is #{distance}\n"
				else
					puts "	! incorrect, distance to #{user} should be #{distance}, your value was: #{responsehash[user][1]}\n"
					return false
				end
			else
				return false
			end
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
				puts "--> improper format:  OK <source> <next-hop> ..."
				return false
			end

			# lets get rid of the "OK" and use the same functionality to create a new hash of return vals
			if(data.size==0)
				puts "--> Nil response??"
				return false
			end

			data[0] = data[0].delete "OK"
			srnexthops=data[0]	# save old value in string format for easy printing
			rnexthopshash = Hash.new
			data[0].split(' ').each { |i| rnexthopshash[i.to_i] = nil }

			if(nexthopshash.keys.sort != rnexthopshash.keys.sort)
				puts "--> Invalid next hops: #{srnexthops}  Should have been:  #{snexthops}"
				return false
			else
				puts "--> Next hops were correct!"
			end

			return true

		end
		
		def printdata()
			data=recv_data_from_rdaemon(1)
			data.each { |x| puts x}
		end

		def checksize(data, size)
			
			if(data.size==0)
				puts "--> Nil response??"
				return false
			end
			(oks,sizes)=data[0].split
			if(oks == "OK" && sizes.to_i==size)
				puts "--> correct size of user table, #{size}"
			else
				puts "--> incorrect size of user table, should be OK #{size}, you had: #{data[0]}"
				return false				
			end

			if(data.size==size+1)
				puts "--> correct, # of entries in response matches OK #"
				return true
			else
				puts "--> incorrect, # of entries in response does not match OK #"
				return false
			end
		end
		
		def checknexthops(data, chan, source, nexthops)
			# The nexthops can be returned in any order, so lets toss them in a hash
			nexthopshash = Hash.new
			nexthops.split(' ').each { |i| nexthopshash[i.to_i] = nil }

			index = 0

			while(index < data.size)
				if(data[index] =~ /^#{chan} #{source}/)	
					(rchan, rsource, rnexthops) = data[index].split(" ", 3)
					rnexthopshash = Hash.new
					rnexthops.split(' ').each { |i| rnexthopshash[i.to_i] = nil }

					if(nexthopshash.keys.sort == rnexthopshash.keys.sort)
#						data.delete_at(index)
						puts "--> Next hops were correct for #{chan} from #{source} with next hops: #{nexthops}"
						return true
					end
				end
				
				index = index + 1
			end

			puts "--> Could not find next hops for #{chan} from #{source} with next hops: #{nexthops}"
			return false
		end
		
		
end
