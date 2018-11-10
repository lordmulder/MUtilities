#!/bin/bash
header=0
while IFS='' read -r line; do
	if [ $((header)) -lt 1 ]; then
		if [[ "${line}" =~ "known_hosts" ]]; then
			((header=header+1))
		fi
	else
		host=$(grep -iPo '[A-Za-z0-9.\-]+' <<<$line | head -1)
		if [[ -z "${host}" || "${host}" == "NULL" || "${host}" == "null" ]]; then
			continue;
		fi
		if [ -n "${host}" ]; then
			echo -e "\n\033[1;37m---- ${host} ---\033[0m"
			output=$(curl -vfk -o /dev/null --max-time 30 -A "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:63.0) Gecko/20100101 Firefox/63.0" "http://${host}/" 2>&1)
			status=$(grep -iPo 'HTTP/.+\s+\K\d+' <<<$output  | head -1)
			if [ $((status)) -eq 200 ]; then
				echo -e "\033[1;32mOkay.\033[0m"
			else
				if [[ $((status)) -gt 300 && $((status)) -lt 304 ]] || [[ $((status)) -gt 306 && $((status)) -lt 309 ]]; then
					location=$(grep -iPo 'Location:\s*\K[[:graph:]]+' <<<$output | head -1)
					if [ -n "${location}" ]; then
						redir=$(grep -iPo 'https?://\K[A-Za-z0-9.\-]+' <<<$location)
						if [ -n "${redir}" ]; then
							if [[ "${redir}" == "${host}" ]]; then
								if [[ "${location}" =~ "https:" ]]; then
									echo -e "\033[1;36mRedirected to HTTP (\"${redir}\")\033[0m"
								else
									echo -e "\033[1;33mRedirected to other URL: ${location}\033[0m"
									echo -e "\n${output}"
								fi
							else
								echo -e "\033[1;33mRedirected to other host: ${redir}\033[0m"
								echo -e "\n${output}"
							fi
						else
							if [[ "${location}" =~ "" ]]; then
								echo -e "\033[1;33mInternal redirect: ${location}\033[0m"
								echo -e "\n${output}"
							else
								echo -e "\033[1;31mStrange redirect URL: ${location}\033[0m"
								echo -e "\n${output}"
							fi
						fi
					else
						echo -e "\033[1;31mFailed to get redirect URL!\033[0m"
						echo -e "\n${output}"
					fi
				else
					echo -e "\033[1;31mFailed with status $((status)) !!!\033[0m"
					echo -e "\n${output}"
				fi
			fi
		fi
	fi
done < "$1"
