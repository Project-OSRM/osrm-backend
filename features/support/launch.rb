require 'socket'
require 'sys/proctable'

LAUNCH_TIMEOUT = 5
SHUTDOWN_TIMEOUT = 5

class OSRMLauncher
  def initialize &block
    Dir.chdir TEST_FOLDER do
      begin
        begin
          Timeout.timeout(LAUNCH_TIMEOUT) do
            osrm_up
            wait_for_connection
          end
        rescue Timeout::Error
          raise "*** Launching osrm-routed timed out."
        end
        yield
      ensure
        begin
          Timeout.timeout(SHUTDOWN_TIMEOUT) do
            osrm_down
          end
        rescue Timeout::Error
          raise "*** Shutting down osrm-routed timed out."
        end
      end
    end
  end
end

def osrm_up?
  if @pipe
    begin
      Process.getpgid @pipe.pid
      true
    rescue Errno::ESRCH
      false
    end
  else
    false
  end
end

def osrm_up
  return if osrm_up?
  #exec avoids popen running osrm-routed inside a shell
  #if the cmd is run inside a shell, popen returns the pid for the shell, and if we try to kill it,
  #the child process is orphaned, and we can't terminate it.
  @pipe = IO.popen('exec ../osrm-routed 1>>osrm-routed.log 2>>osrm-routed.log')
end

def osrm_down
  if @pipe
    Process.kill 'TERM', @pipe.pid
    wait_for_shutdown
    @pipe = nil
  end
end

def kill
  if @pipe
    Process.kill 'KILL', @pipe.pid
  end
end

def wait_for_connection
  while true
    begin
      socket = TCPSocket.new('localhost', OSRM_PORT)
      return
    rescue Errno::ECONNREFUSED
      sleep 0.1
    end
  end
end

def wait_for_shutdown
  while osrm_up?
    sleep 0.1
  end
end
