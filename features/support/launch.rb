require 'socket'
require 'open3'

OSRM_ROUTED_LOG_FILE = 'osrm-routed.log'

class OSRMBackgroundLauncher
  def initialize (input_file, use_ele=false, &block)
    @input_file = input_file
    Dir.chdir TEST_FOLDER do
      begin
        launch use_ele
        yield
      ensure
        shutdown
      end
    end
  end

  private

  def launch use_ele
    Timeout.timeout(OSRM_TIMEOUT) do
      osrm_up use_ele
      wait_for_connection
    end
  rescue Timeout::Error
    raise RoutedError.new "Launching osrm-routed timed out."
  end

  def shutdown
    Timeout.timeout(OSRM_TIMEOUT) do
      osrm_down
    end
  rescue Timeout::Error
    kill
    raise RoutedError.new "Shutting down osrm-routed timed out."
  end


  def osrm_up?
    if @pid
      `ps -o state -p #{@pid}`.split[1].to_s =~ /^[DRST]/
    else
      false
    end
  end

  def osrm_up use_ele=false
    return if osrm_up?
    @pid = Process.spawn("#{BIN_PATH}/osrm-routed #{'-e 1' if use_ele} #{@input_file} --port #{OSRM_PORT}",:out=>OSRM_ROUTED_LOG_FILE, :err=>OSRM_ROUTED_LOG_FILE)
    Process.detach(@pid)    # avoid zombie processes
  end

  def osrm_down
    if @pid
      Process.kill 'TERM', @pid
      wait_for_shutdown
    end
  end

  def kill
    if @pid
      Process.kill 'KILL', @pid
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
end
