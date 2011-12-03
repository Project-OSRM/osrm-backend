require 'socket'

class OSRMLauncher
  def initialize &block
    Dir.chdir TEST_FOLDER do
      osrm_up
      yield
      osrm_down
    end
  end
end

def each_process name, &block
  process_list = `ps -o pid -o state -o ucomm`
  process_list.scan /(\d+)\s+([^\s]*)\s+(#{name})/ do |pid,state|
  yield pid.to_i, state.strip if ['I','R','S','T'].include? state[0]
end
end

def osrm_up?
  find_pid('osrm-routed') != nil
end

def find_pid name
  each_process(name) { |pid,state| return pid.to_i }
  return nil
end

def osrm_up
  return if osrm_up?
  pipe = IO.popen('../osrm-routed 1>>osrm-routed.log 2>>osrm-routed.log')
  timeout = 5
  (timeout*10).times do
    begin
      socket = TCPSocket.new('localhost', 5000)
      socket.puts 'ping'
    rescue Errno::ECONNREFUSED
      sleep 0.1
    end
  end
  sleep 0.1
end

def osrm_down
  each_process('osrm-routed') { |pid,state| Process.kill 'TERM', pid }
  each_process('osrm-prepare') { |pid,state| Process.kill 'TERM', pid }
  each_process('osrm-extract') { |pid,state| Process.kill 'TERM', pid }
  wait_for_shutdown 'osrm-routed'
  wait_for_shutdown 'osrm-prepare'
  wait_for_shutdown 'osrm-extract'  
end

def osrm_kill
  each_process('osrm-routed') { |pid,state| Process.kill 'KILL', pid }
  each_process('osrm-prepare') { |pid,state| Process.kill 'KILL', pid }
  each_process('osrm-extract') { |pid,state| Process.kill 'KILL', pid }
  wait_for_shutdown 'osrm-routed'
  wait_for_shutdown 'osrm-prepare'
  wait_for_shutdown 'osrm-extract'  
end

def wait_for_shutdown name
  timeout = 10
  (timeout*10).times do
    return if find_pid(name) == nil
    sleep 0.1
  end
  raise "*** Could not terminate #{name}."
end
