#! /usr/bin/ruby

class IRC

    def initialize(server, port, nick, channel)
        @server = server
        @port = port
        @nick = nick
        @channel = channel
    end

    def recv_data_from_server (timeout)
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
            ready = select([@irc], nil, nil, 0.0001)
            next if !ready
            for s in ready[0]
                if s == @irc then
                    next if @irc.eof
                    s = @irc.gets
										puts s
                    received_data[k] = s
                    k= k + 1
                end
            end
        end
        return received_data
    end

    def test_silence(timeout)
        data=recv_data_from_server(timeout)
        if (data.size > 0)
            return false
        else
            return true
        end
    end
    
    def send(s)
        # Send a message to the irc server and print it to the screen
        puts "--> #{s}"
        @irc.send "#{s}\n", 0 
    end

		def send_junk(total)
				# send a specified amount of junk
				while(total > 0)
					@irc.send "g", 0
					total -= 1
				end

		end

    def connect()
        # Connect to the IRC server
        @irc = TCPSocket.open(@server, @port)
    end

    def disconnect()
        @irc.close
    end

    def send_nick(s)
        send("NICK #{s}")
    end
    
    def send_user(s)
        send("USER #{s}")
    end

    def quit_user(s)
	send("QUIT #{s}")
    end

    def get_motd
        data = recv_data_from_server(1)
        ## CHECK data here
        
        if(data[0] =~ /^:[^ ]+ *375 *gnychis *:- *[^ ]+ *Message of the day - *.\n/)
            puts "\tRPL_MOTDSTART 375 correct"
        else
            puts "\tRPL_MOTDSTART 375 incorrect"
            return false
        end
        
        k = 1
        while ( k < data.size-1)
            
            if(data[k] =~ /:[^ ]+ *372 *gnychis *:- *.*/)
                puts "\tRPL_MOTD 372 correct"
            else
                puts "\tRPL_MOTD 372 incorrect"
                return false
            end
            k = k + 1
        end

        if(data[data.size-1] =~ /:[^ ]+ *376 *gnychis *:End of \/MOTD command/)
            puts "\tRPL_ENDOFMOTD 376 correct"
        else
            puts "\tRPL_ENDOFMOTD 376 incorrect"
            return false
        end
        
        return true
    end

    def send_privmsg(s1, s2)
	send("PRIVMSG #{s1} :#{s2}")
    end
        
    def raw_join_channel(joiner, channel)
        send("JOIN #{channel}")
        ignore_reply()
    end

		def join_chan(channel)
				send("JOIN #{channel}")
		end
    
    def join_channel(joiner, channel)
        send("JOIN #{channel}")
        
        data = recv_data_from_server(1);
        if(data[0] =~ /^:#{joiner} *JOIN *#{channel}/)
            puts "\tJOIN echoed back"
        else
            puts "\tJOIN was not echoed back to the client"
            return false
        end
        
        if(data[1] =~ /^:[^ ]+ *353 *#{joiner} *= *#{channel} *:.*#{joiner}/)
            puts "\tRPL_NAMREPLY 353 correct"
        else
            puts "\tRPL_NAMREPLY 353 incorrect"
            return false
        end
        
        if(data[2] =~ /^:[^ ]+ *366 *#{joiner} *#{channel} *:End of \/NAMES list/)
            puts "\tRPL_ENDOFNAMES 366 correct"
        else
            puts "\tRPL_ENDOFNAMES 366 incorrect"
            return false
        end
        
        return true
    end

    def who(s)
        send("WHO #{s}")
        
        data = recv_data_from_server(1);
        
        if(data[0] =~ /^:[^ ]+ *352 *gnychis *#{s} *please *[^ ]+ *[^ ]+ *gnychis *H *:0 *The MOTD/)
            puts "\tRPL_WHOREPLY 352 correct"
        else
            puts "\tRPL_WHOREPLY 352 incorrect"
            return false
        end

        if(data[1] =~ /^:[^ ]+ *315 *gnychis *#{s} *:End of \/WHO list/)
            puts "\tRPL_ENDOFWHO 315 correct"
        else
            puts "\tRPL_ENDOFWHO 315 incorrect"
            return false
        end
        return true
    end
    
    def list
	send("LIST")
        
        data = recv_data_from_server(1);
        if(data[0] =~ /^:[^ ]+ *321 *gnychis *Channel *:Users Name/)
            puts "\tRPL_LISTSTART 321 correct"
        else
            puts "\tRPL_LISTSTART 321 incorrect"
            return false
        end
        
        if(data[1] =~ /^:[^ ]+ *322 *gnychis *#linux *:1/)
            puts "\tRPL_LIST 322 correct"
        else
            puts "\tRPL_LIST 322 incorrect"
            return false
        end
        
        if(data[2] =~ /^:[^ ]+ *323 *gnychis *:End of \/LIST/)
            puts "\tRPL_LISTEND 323 correct"
        else
            puts "\tRPL_LISTEND 323 incorrect"
            return false
        end
        
        return true
    end
    
    def checkmsg(from, to, msg)
			reply_matches(/^:#{from} *PRIVMSG *#{to} *:#{msg}/, "PRIVMSG")
    end
    
    def check2msg(from, to1, to2, msg)
        data = recv_data_from_server(1);
        if((data[0] =~ /^:#{from} *PRIVMSG *#{to1} *:#{msg}/ && data[1] =~ /^:#{from} *PRIVMSG *#{to2} *:#{msg}/) ||
           (data[1] =~ /^:#{from} *PRIVMSG *#{to1} *:#{msg}/ && data[0] =~ /^:#{from} *PRIVMSG *#{to2} *:#{msg}/))
            puts "\tPRIVMSG to #{to1} and #{to2} correct"
            return true
        else
            puts "\tPRIVMSG to #{to1} and #{to2} incorrect"
            return false
        end
    end
    
    def check_echojoin(from, channel)
	reply_matches(/^:#{from} *JOIN *#{channel}/,
			    "Test if first client got join echo")
    end
    
    def part_channel(parter, channel)
        send("PART #{channel}")
	reply_matches(/^:#{parter}![^ ]+@[^ ]+ *QUIT *:/)
    end

    def check_part(parter, channel)
	reply_matches(/^:#{parter}![^ ]+@[^ ]+ *QUIT *:/)
    end

    def ignore_reply
        recv_data_from_server(1)
    end

    def reply_matches(rexp, explanation = nil)
	data = recv_data_from_server(1)
	if (rexp =~ data[0])
	    puts "\t #{explanation} correct" if explanation
	    return true
	else
	    puts "\t #{explanation} incorrect: #{data[0]}" if explanation
	    return false
	end
    end

		def check_quit()
			send("QUIT")
			sleep(1)
			return @irc.closed?
		end

		def check_conn()
			if(recv_data_from_server(1)==nil)
				return false
			else
				return true
			end
		end

end

