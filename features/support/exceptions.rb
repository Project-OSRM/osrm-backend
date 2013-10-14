
class OSRMError < StandardError
  attr_accessor :msg, :code, :process

  def initialize process, code, msg, log, lines
    @process = process
    @code = code
    @msg = msg
    @lines = lines
    @log = log
    @extract = log_tail @log, @lines
  end

  def to_s
    "*** #{@msg}\nLast #{@lines} lines from #{@log}:\n#{@extract}\n"
  end

  private

  def log_tail path, n
    File.open(path) do |f|
      return f.tail(n).map { |line| "  > #{line}" }.join "\n"
    end
  end
end

class OsmosisError < OSRMError
  def initialize code, msg
    super 'osmosis', code, msg, PREPROCESS_LOG_FILE, 40
  end
end

class ExtractError < OSRMError
  def initialize code, msg
    super 'osrm-extract', code, msg, PREPROCESS_LOG_FILE, 3
  end
end

class PrepareError < OSRMError
  def initialize code, msg
    super 'osrm-prepare', code, msg, PREPROCESS_LOG_FILE, 3
  end
end

class RoutedError < OSRMError
  def initialize msg
    super 'osrm-routed', nil, msg, OSRM_ROUTED_LOG_FILE, 3
  end
end
