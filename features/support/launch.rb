require 'socket'
require 'open3'

if ENV['OS']==/Windows.*/ then
  TERMSIGNAL='TERM'
else
  TERMSIGNAL=9
end

OSRM_ROUTED_LOG_FILE = 'osrm-routed.log'

class OSRMLoader
  
  @@pid = nil
    
  def self.load input_file, &block
    @input_file = input_file
    Dir.chdir TEST_FOLDER do
      self.load_data
      self.launch unless @@pid
      yield
    end
  end
  
  def self.load_data
    run_bin "osrm-datastore", @input_file
  end

  def self.launch
    Timeout.timeout(LAUNCH_TIMEOUT) do
      self.osrm_up
      self.wait_for_connection
    end
  rescue Timeout::Error
    raise RoutedError.new "Launching osrm-routed timed out."
  end

  def self.shutdown
    Timeout.timeout(SHUTDOWN_TIMEOUT) do
      self.osrm_down
    end
  rescue Timeout::Error
    self.kill
    raise RoutedError.new "Shutting down osrm-routed timed out."
  end

  def self.osrm_up?
      if @@pid
      begin
        if Process.waitpid(@@pid, Process::WNOHANG) then
           false
        else
           true
        end
      rescue Errno::ESRCH, Errno::ECHILD
        false
      end
    end
  end

  def self.osrm_up
    return if self.osrm_up?
    @@pid = Process.spawn("#{BIN_PATH}/osrm-routed --sharedmemory=1 --port #{OSRM_PORT}",:out=>OSRM_ROUTED_LOG_FILE, :err=>OSRM_ROUTED_LOG_FILE)
    Process.detach(@@pid)    # avoid zombie processes
  end

  def self.osrm_down
    if @@pid
      Process.kill TERMSIGNAL, @@pid
      self.wait_for_shutdown
    end
  end

  def self.kill
    if @@pid
      Process.kill 'KILL', @@pid
    end
  end

  def self.wait_for_connection
    while true
      begin
        socket = TCPSocket.new('127.0.0.1', OSRM_PORT)
        return
      rescue Errno::ECONNREFUSED
        sleep 0.1
      end
    end
  end

  def self.wait_for_shutdown
    while self.osrm_up?
      sleep 0.1
    end
  end
end
