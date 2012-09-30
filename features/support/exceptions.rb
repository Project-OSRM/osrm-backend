
class OSRMError < StandardError
  attr_accessor :process, :code, :msg
  
  def initialize process, code, msg
    @process = process
    @code = code
    @msg = msg
  end
  
  def to_s
    @msg
  end
end