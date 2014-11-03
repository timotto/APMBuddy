// TODO Do stats counting here

void on_rx(int linkId, mavlink_message_t* msg)
{
//  Serial.print("r");
}

void on_tx(int linkId, mavlink_message_t* msg)
{
//  Serial.print("t");
}

void on_duplicate(int linkId, mavlink_message_t* msg)
{
  Serial.print("dup@");
  Serial.print(linkId);
  Serial.print(":");
  Serial.println(msg->msgid);
}

