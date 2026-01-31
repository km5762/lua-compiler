local player1_score = 9
local player2_score = 12

if player1_score >= 20 and player2_score >= 20 then
  print(1)
elseif player1_score > player2_score then
  print(2)
elseif player1_score < player2_score then
  print(3)
else
  print(4)
end
