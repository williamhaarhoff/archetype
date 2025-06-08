
clean:
  rm ./build/* -rf 

build: 
  if [ ! -d ./build ]; then mkdir build; fi && cd ./build && cmake .. && make && just intellisense

intellisense:
    jq -s add build/compile_commands.json > compile_commands.json 

run:
  ./build/example